#include "NluSlots.h"
#include "../AssistantNumbers.h"
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <cmath>
#include <stdexcept>
namespace cockpit {
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
double number(QString text) {
    text.remove(QStringLiteral("百分之")).remove('%').remove(QChar(0xff05));
    text = text.trimmed();
    static const QRegularExpression literal(
        QStringLiteral("^(?:\\d+(?:\\.\\d+)?|(?:一百|[一二两三四五六七八九]?十[一二两三四五六七八九]?|["
                       "零一二两三四五六七八九])(?:点[零一二两三四五六七八九]+)?)$"));
    require(literal.match(text).hasMatch(), "unrecognized numeric slot");
    bool ok = false;
    const auto value = text.toDouble(&ok);
    if (ok)
        return value;
    const auto parts = text.split(QChar(u'点'));
    const int whole = assistantCommandInteger(parts[0]);
    require(whole >= 0 && parts.size() <= 2, "unrecognized numeric slot");
    double result = whole, place = 0.1;
    if (parts.size() == 2)
        for (auto ch : parts[1]) {
            const int digit = assistantCommandInteger(QString(ch));
            require(digit >= 0 && digit <= 9, "invalid decimal");
            result += place * digit;
            place *= 0.1;
        }
    return result;
}
} // namespace
QJsonObject normalizeSlots(const QString &text, const QString &input, const QString &act,
                           const QString &intent, const QJsonArray &entities) {
    static const QMap<QString, QPair<QString, QSet<QString>>> schemas{
        {"WINDOW_SET", {"EXECUTE", {"TARGET", "VALUE", "ACTION", "UNIT"}}},
        {"WINDOW_QUERY", {"QUERY", {"TARGET"}}},
        {"BATTERY_QUERY", {"QUERY", {}}},
        {"TEMPERATURE_SET", {"EXECUTE", {"ZONE", "VALUE", "UNIT"}}},
        {"AC_SET", {"EXECUTE", {"ACTION"}}},
        {"FAN_SET", {"EXECUTE", {"VALUE", "UNIT"}}},
        {"MEDIA_CONTROL", {"EXECUTE", {"ACTION"}}},
        {"RADIO_TUNE", {"EXECUTE", {"STATION"}}}};
    require(schemas.contains(intent) && schemas[intent].first == act, "speech act and intent disagree");
    QMap<QString, QString> slotValues;
    const QRegularExpression numeric(
        QStringLiteral("[-+负正]?(?:百分之)?(?:\\d+(?:\\.\\d+)?|[零一二两三四五六七八九十百]+(?:点["
                       "零一二三四五六七八九]+)?)[%％]?"));
    const auto points = text.toUcs4();
    for (const auto &entry : entities) {
        const auto e = entry.toObject();
        const auto label = e["label"].toString();
        require(schemas[intent].second.contains(label), "extraneous slots for intent; no partial command");
        require(!slotValues.contains(label), "multiple same-type slots require clarification");
        slotValues[label] = e["text"].toString();
        if (label == "VALUE") {
            const int a = e["start"].toInt(), b = e["end"].toInt();
            const int left = QString::fromUcs4(points.data(), a).size(),
                      right = QString::fromUcs4(points.data(), b).size();
            auto matches = numeric.globalMatch(text);
            int overlaps = 0;
            bool exact = false;
            while (matches.hasNext()) {
                auto m = matches.next();
                if (m.capturedStart() < right && m.capturedEnd() > left) {
                    ++overlaps;
                    exact = m.capturedStart() == left && m.capturedEnd() == right;
                }
            }
            require(overlaps == 1 && exact, "numeric span omits part of original literal");
        }
    }
    auto slot = [&](const char *key) {
        require(slotValues.contains(key), "missing required slot");
        return slotValues[key];
    };
    static const QMap<QString, QString> targets{
        {"左前", "leftFront"},    {"驾驶位", "leftFront"}, {"主驾驶", "leftFront"}, {"右前", "rightFront"},
        {"副驾驶", "rightFront"}, {"左后", "leftRear"},    {"右后", "rightRear"},   {"全部", "all"},
        {"所有", "all"},          {"四个", "all"},         {"全车", "all"}};
    if (intent == "BATTERY_QUERY") {
        require(QRegularExpression(
                    QStringLiteral(
                        "电量|电池|电能|剩.{0,6}电|还有.{0,6}电|电.{0,5}剩|(?<![a-z0-9_])soc(?![a-z0-9_])"),
                    QRegularExpression::CaseInsensitiveOption)
                    .match(text)
                    .hasMatch(),
                "battery query requires battery evidence");
        return {};
    }
    if (intent == "WINDOW_SET" || intent == "WINDOW_QUERY") {
        const auto target = targets.value(slot("TARGET"));
        require(!target.isEmpty(), "unknown window");
        if (intent == "WINDOW_QUERY") {
            require(QRegularExpression(QStringLiteral("窗|玻璃")).match(text).hasMatch(),
                    "window query requires window evidence");
            require(QRegularExpression(QStringLiteral("看|查|问|说下|告诉|报|知道|显示|多少|多大|是否|有没有|"
                                                      "是不是|吗|没[？?]?$|开度|开合状态|打开情况|关严"))
                        .match(text)
                        .hasMatch(),
                    "window query requires a question or state request");
            return {{"target", target}};
        }
        double value = 0;
        if (slotValues.contains("VALUE"))
            value = number(slotValues["VALUE"]);
        else {
            const auto action = slot("ACTION");
            const QStringList closed{QStringLiteral("关"),   QStringLiteral("关闭"), QStringLiteral("关上"),
                                     QStringLiteral("关严"), QStringLiteral("关掉"), QStringLiteral("关好"),
                                     QStringLiteral("全关"), QStringLiteral("关了")};
            const QStringList open{QStringLiteral("打开"), QStringLiteral("全开"), QStringLiteral("开启"),
                                   QStringLiteral("开"),   QStringLiteral("降下"), QStringLiteral("开起来"),
                                   QStringLiteral("开着")};
            require(closed.contains(action) || open.contains(action), "unknown window action");
            value = open.contains(action) ? 100 : 0;
        }
        require(value >= 0 && value <= 100, "window outside 0..100");
        return {{"target", target}, {"action", "set"}, {"value", value}, {"unit", "percent"}};
    }
    if (intent == "TEMPERATURE_SET" || intent == "FAN_SET") {
        const auto literal = slot("VALUE");
        require(!literal.contains('%') && !literal.contains(QChar(0xff05)) &&
                    !literal.contains(QStringLiteral("百分之")),
                "invalid unit");
        const double value = number(literal);
        const auto unit = slotValues.value("UNIT");
        if (intent == "FAN_SET") {
            require(value >= 0 && value <= 7 && value == std::round(value) &&
                        (unit.isEmpty() || unit == QStringLiteral("档") || unit == QStringLiteral("挡")),
                    "fan outside 0..7");
            return {{"value", value}};
        }
        static const QMap<QString, QString> zones{
            {"主驾", "driver"},    {"主驾驶", "driver"},    {"主价", "driver"},
            {"主架", "driver"},    {"驾驶位", "driver"},    {"驾驶员", "driver"},
            {"副驾", "passenger"}, {"副驾驶", "passenger"}, {"乘客侧", "passenger"}};
        const auto zone = slot("ZONE");
        if (zone == QStringLiteral("主价") || zone == QStringLiteral("主架"))
            require(
                input == "asr" &&
                    QRegularExpression(QStringLiteral("(调到|调至|设成|设到|设为|改成|调成).+度[。！!？?]?$"))
                        .match(text)
                        .hasMatch(),
                "homophone zone requires explicit ASR temperature command");
        require(zones.contains(zone) && value >= 16 && value <= 30 && value * 2 == std::round(value * 2) &&
                    (unit.isEmpty() || unit == QStringLiteral("度") || unit == QStringLiteral("摄氏度")),
                "zone or temperature invalid");
        return {{"zone", zones[zone]}, {"value", value}, {"unit", "celsius"}};
    }
    if (intent == "AC_SET") {
        const auto action = slot("ACTION");
        const QStringList open{QStringLiteral("打开"), QStringLiteral("全开"), QStringLiteral("开启"),
                               QStringLiteral("开"),   QStringLiteral("降下"), QStringLiteral("开起来"),
                               QStringLiteral("开着")};
        const QStringList closed{QStringLiteral("关"),   QStringLiteral("关闭"), QStringLiteral("关上"),
                                 QStringLiteral("关严"), QStringLiteral("关掉"), QStringLiteral("关好"),
                                 QStringLiteral("全关"), QStringLiteral("关了")};
        require(open.contains(action) || closed.contains(action), "unknown AC action");
        return {{"state", open.contains(action)}};
    }
    if (intent == "MEDIA_CONTROL") {
        const auto action = slot("ACTION");
        static const QMap<QString, QString> actions{
            {"暂停", "pause"},      {"停一下", "pause"},     {"停止", "pause"},    {"停下", "pause"},
            {"继续", "play"},       {"播放", "play"},        {"恢复", "play"},     {"接着放", "play"},
            {"下一首", "next"},     {"下一曲", "next"},      {"切下一首", "next"}, {"上一首", "previous"},
            {"上一曲", "previous"}, {"切上一首", "previous"}};
        require(actions.contains(action), "unknown media action");
        return {{"action", actions[action]}};
    }
    const auto station = slot("STATION");
    require(QStringList{QStringLiteral("中国之声"), QStringLiteral("经济之声"), QStringLiteral("音乐之声"),
                        QStringLiteral("香港电台第一台")}
                .contains(station),
            "unknown station");
    return {{"stationName", station}};
}
} // namespace cockpit
