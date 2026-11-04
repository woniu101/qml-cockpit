#include "AssistantVehicleSetting.h"
#include "AssistantNumbers.h"

#include <QRegularExpression>
#include <cmath>

namespace {
const QStringList WindowPositions{QStringLiteral("左前"), QStringLiteral("右前"),
                                  QStringLiteral("左后"), QStringLiteral("右后")};
const QStringList ClimateZones{QStringLiteral("主驾"), QStringLiteral("副驾")};

QString normalized(QString text) {
    static const QRegularExpression spacing(QStringLiteral("\\s+"));
    text.remove(spacing);
    while (!text.isEmpty() && QStringLiteral("。！!？?").contains(text.back()))
        text.chop(1);
    return text;
}
}

QStringList AssistantVehicleSetting::targets() const {
    if (kind == Kind::Temperature)
        return ClimateZones;
    auto labels = WindowPositions;
    for (auto &label : labels)
        label += QStringLiteral("窗");
    return labels;
}

QString AssistantVehicleSetting::prompt() const {
    if (relative)
        return kind == Kind::Temperature ? QStringLiteral("要调整哪个温区？") : QStringLiteral("要调整哪扇车窗？");
    if (kind == Kind::Temperature)
        return QStringLiteral("要把哪个温区设为 %1 度？").arg(value);
    return QStringLiteral("要把哪扇车窗开到 %1%？").arg(value);
}

QString AssistantVehicleSetting::rangeError() const {
    if (relative) {
        const double amount = std::abs(value);
        if (kind == Kind::Temperature)
            return amount < 0.5 || amount > 14 ? QStringLiteral("温度调整量应为 0.5 到 14 度") : QString();
        return amount < 1 || amount > 100 ? QStringLiteral("车窗调整量应为 1 到 100 个百分点") : QString();
    }
    if (kind == Kind::Temperature)
        return value < 16 || value > 30 ? QStringLiteral("空调设定温度范围是 16 到 30 度") : QString();
    return value < 0 || value > 100 ? QStringLiteral("车窗开度范围是 0 到 100%") : QString();
}

int AssistantVehicleSetting::targetFromReply(const QString &text) const {
    static const QRegularExpression zone(QStringLiteral("^(?:选|就|用)?(主驾|副驾)(?:驶)?(?:这边)?$"));
    static const QRegularExpression window(QStringLiteral("^(?:选|就|用)?(左前|右前|左后|右后)(?:车窗|窗|窗户)?$"));
    const auto match = (kind == Kind::Temperature ? zone : window).match(normalized(text));
    if (!match.hasMatch())
        return -1;
    return int((kind == Kind::Temperature ? ClimateZones : WindowPositions).indexOf(match.captured(1)));
}

std::optional<AssistantVehicleSetting> assistantVehicleSetting(const QString &request, bool spoken) {
    using Kind = AssistantVehicleSetting::Kind;
    const auto phrase = normalized(request);
    static const QRegularExpression temperature(QStringLiteral(
        "^(?:请帮我|请|帮我)?(?:把)?(?:(主驾(?:驶)?|副驾(?:驶)?|主[价架])(?:空调)?(?:温度)?|空调(?:温度)?|温度)"
        "(?:调到|调至|设为)([0-9]{1,2}(?:\\.[05])?|[二三]?十[一二三四五六七八九]?)度$"));
    const auto setting = temperature.match(phrase);
    if (setting.hasMatch()) {
        const auto zone = setting.captured(1);
        if (!spoken && (zone == QStringLiteral("主价") || zone == QStringLiteral("主架")))
            return {};
        const auto value = setting.captured(2);
        bool numeric = false;
        double target = value.toDouble(&numeric);
        if (!numeric)
            target = assistantCommandInteger(value);
        return AssistantVehicleSetting{Kind::Temperature, zone.isEmpty() ? -1 : zone.startsWith(QChar(u'副')) ? 1 : 0, target};
    }

    static const QRegularExpression opening(QStringLiteral(
        "^(?:请帮我|请|帮我)?(?:把)?(左前|右前|左后|右后)?(?:车窗|窗户|窗)(?:开到|打开到|降到|调到)"
        "(?:百分之([0-9零〇一二两三四五六七八九十百]{1,4})|([0-9]{1,3})[%％])$"));
    const auto window = opening.match(phrase);
    if (window.hasMatch()) {
        const auto value = window.captured(2).isEmpty() ? window.captured(3) : window.captured(2);
        return AssistantVehicleSetting{Kind::Window, int(WindowPositions.indexOf(window.captured(1))),
                                       double(assistantCommandInteger(value))};
    }
    const auto closed = assistantWindowCloseTarget(phrase);
    if (closed && *closed >= 0)
        return AssistantVehicleSetting{Kind::Window, *closed, 0};
    return {};
}

std::optional<int> assistantWindowCloseTarget(const QString &request) {
    static const QRegularExpression close(QStringLiteral(
        "^(?:请帮我|请|帮我)?(?:(?:关闭|关掉|关上|关)(左前|右前|左后|右后|所有|全部|全车)(?:车窗|窗)"
        "|把(左前|右前|左后|右后|所有|全部|全车)(?:车窗|窗)(?:关闭|关掉|关上)|全(?:部|车)关窗)$"));
    const auto match = close.match(request);
    if (!match.hasMatch())
        return {};
    const auto position = match.captured(1).isEmpty() ? match.captured(2) : match.captured(1);
    return int(WindowPositions.indexOf(position));
}

std::optional<AssistantVehicleSetting> assistantRelativeVehicleSetting(const QString &request) {
    using Kind = AssistantVehicleSetting::Kind;
    const auto phrase = normalized(request);
    static const QRegularExpression temperature(QStringLiteral(
        "^再(调高|调低|升高|降低|升|降|高|低)(?:一?点|([0-9]{1,2}(?:\\.[05])?|[零一二两三四五六七八九十]{1,3}|半)度)$"));
    const auto zone = temperature.match(phrase);
    if (zone.hasMatch()) {
        const auto quantity = zone.captured(2);
        bool numeric = false;
        double amount = quantity.toDouble(&numeric);
        if (!numeric)
            amount = quantity.isEmpty() || quantity == QStringLiteral("半") ? 0.5 : assistantCommandInteger(quantity);
        const auto action = zone.captured(1);
        const bool higher = action.endsWith(QChar(u'高')) || action == QStringLiteral("升");
        return AssistantVehicleSetting{Kind::Temperature, -1, amount < 0 ? 0 : higher ? amount : -amount, true};
    }

    static const QRegularExpression window(QStringLiteral(
        "^再(开|打开|关|关上)(?:一?点|百分之([0-9零〇一二两三四五六七八九十百]{1,4})"
        "|([0-9]{1,3})[%％]|([0-9零〇一二两三四五六七八九十百]{1,4})个?百分点)$"));
    const auto opening = window.match(phrase);
    if (!opening.hasMatch())
        return {};
    auto quantity = opening.captured(2);
    if (quantity.isEmpty())
        quantity = opening.captured(3);
    if (quantity.isEmpty())
        quantity = opening.captured(4);
    const int amount = quantity.isEmpty() ? 10 : assistantCommandInteger(quantity);
    const bool open = opening.captured(1).endsWith(QChar(u'开'));
    return AssistantVehicleSetting{Kind::Window, -1, double(amount < 0 ? 0 : open ? amount : -amount), true};
}

bool assistantCancelsRequest(const QString &request) {
    static const QRegularExpression cancel(QStringLiteral(
        "(?:^|[，,。；;])(?:还是|先)?(?:取消(?:这次)?|算了|不用了|不调了|不做了)$"));
    return cancel.match(normalized(request)).hasMatch();
}
