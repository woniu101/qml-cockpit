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

double temperatureNumber(const QString &text) {
    bool numeric = false;
    const auto value = text.toDouble(&numeric);
    if (numeric)
        return value;
    const auto parts = text.split(QChar(u'点'));
    const int whole = assistantCommandInteger(parts.front());
    if (whole < 0 || parts.size() > 2)
        return -1;
    if (parts.size() == 1)
        return whole;
    double decimal = 0;
    double place = 0.1;
    for (const auto digit : parts.back()) {
        const int value = assistantCommandInteger(QString(digit));
        if (value < 0 || value > 9)
            return -1;
        decimal += place * value;
        place /= 10;
    }
    return whole + decimal;
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
        return value < 16 || value > 30 ? QStringLiteral("空调设定温度范围是 16 到 30 度")
            : !qFuzzyCompare(value * 2, std::round(value * 2)) ? QStringLiteral("温度可按 0.5 度调整") : QString();
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
        "^(?:请帮我|请|帮我|给我?)?(?:把)?(?:(主驾(?:驶)?|副驾(?:驶)?|主[价架])(?:这边|那边)?(?:的)?(?:空调)?(?:温度)?|空调(?:温度)?|温度)?"
        "(?:帮我)?(?:调高到|调低到|升到|降到|调到|调至|调成|调个|设为|设成|设个|设置|定到)([0-9]{1,2}(?:\\.[0-9]{1,2})?|[零一二两三四五六七八九十百]{1,4}(?:点[零一二三四五六七八九]{1,2})?)度(?:吧)?$"));
    const auto setting = temperature.match(phrase);
    if (setting.hasMatch()) {
        const auto zone = setting.captured(1);
        if (!spoken && (zone == QStringLiteral("主价") || zone == QStringLiteral("主架")))
            return {};
        return AssistantVehicleSetting{Kind::Temperature, zone.isEmpty() ? -1 : zone.startsWith(QChar(u'副')) ? 1 : 0,
                                       temperatureNumber(setting.captured(2))};
    }

    static const QRegularExpression opening(QStringLiteral(
        "^(?:请帮我|请|帮我)?(?:把)?(左前|右前|左后|右后)?(?:那边|那扇)?(?:的)?(?:车窗|窗户|窗)(?:开到|打开到|降到|调到)"
        "(.+?)(?:吧)?$"));
    const auto window = opening.match(phrase);
    if (window.hasMatch()) {
        if (const auto value = assistantSettingValue(Kind::Window, window.captured(2), spoken))
            return AssistantVehicleSetting{Kind::Window, int(WindowPositions.indexOf(window.captured(1))), *value};
    }
    const auto closed = assistantWindowCloseTarget(phrase);
    if (closed && *closed >= 0)
        return AssistantVehicleSetting{Kind::Window, *closed, 0};
    return {};
}

std::optional<double> assistantSettingValue(AssistantVehicleSetting::Kind kind, const QString &text, bool spoken) {
    const auto phrase = normalized(text);
    if (kind == AssistantVehicleSetting::Kind::Temperature) {
        static const QRegularExpression temperature(QStringLiteral(
            "^(?:改成|设为|调到)?([0-9]{1,2}(?:\\.[0-9]{1,2})?|[零一二两三四五六七八九十百]{1,4}(?:点[零一二三四五六七八九]{1,2})?)度$"));
        const auto match = temperature.match(phrase);
        return match.hasMatch() ? std::optional<double>(temperatureNumber(match.captured(1))) : std::nullopt;
    }
    static const QRegularExpression opening(QStringLiteral(
        "^(?:改成|设为|调到)?(?:百分之([0-9零〇一二两三四五六七八九十百]{1,5})"
        "|([0-9零〇一二两三四五六七八九十百]{1,5})(%|％|百分号))$"));
    const auto match = opening.match(phrase);
    if (!match.hasMatch() || (!spoken && match.captured(3) == QStringLiteral("百分号")))
        return {};
    const auto value = match.captured(1).isEmpty() ? match.captured(2) : match.captured(1);
    return assistantCommandInteger(value);
}

std::optional<AssistantVehicleSetting> assistantWindowPilotEvidence(const QString &request, bool spoken) {
    static const QRegularExpression grammar(QStringLiteral(
        "^(?:请帮我|请|帮忙|帮我|麻烦)?(?:把|将|让)?(左前|右前|左后|右后)?(?:车窗|窗户|窗)(?:的)?开度"
        "(?:设置为|设为|设成|设到|调到|调至|调为|调整至|改成|定在|定为|定到)"
        "([^，,；;。！？!?]+?)(?:就好|就行|吧)?$"));
    const auto match = grammar.match(normalized(request));
    if (!match.hasMatch())
        return {};
    const auto kind = AssistantVehicleSetting::Kind::Window;
    const auto value = assistantSettingValue(kind, match.captured(2), spoken);
    if (!value)
        return {};
    return AssistantVehicleSetting{kind, int(WindowPositions.indexOf(match.captured(1))), *value};
}

std::optional<AssistantRetarget> assistantRetarget(const QString &request) {
    static const QRegularExpression replace(QStringLiteral(
        "^(?:不对[，,]?|刚才说错了[，,]?)?(?:刚才那扇|刚才那个|刚才那边)?(?:换成|改成|改到|调到)"
        "(?:(左前|右前|左后|右后)(?:车窗|窗|窗户)?|(主驾|副驾)(?:驶|这边)?)$"));
    const auto match = replace.match(normalized(request));
    if (!match.hasMatch())
        return {};
    if (!match.captured(2).isEmpty())
        return AssistantRetarget{AssistantVehicleSetting::Kind::Temperature, int(ClimateZones.indexOf(match.captured(2)))};
    return AssistantRetarget{AssistantVehicleSetting::Kind::Window, int(WindowPositions.indexOf(match.captured(1)))};
}

bool assistantHasMultipleVehicleActions(const QString &request) {
    static const QRegularExpression separator(QStringLiteral("[，,；;]|并(?:且)?|然后|同时|再(?=把|将)"));
    static const QRegularExpression command(QStringLiteral(
        "(?:(?:左前|右前|左后|右后|车窗|窗户).*(?:关上|关掉|关闭|打开|开到|降到)"
        "|(?:设置|设|调到|调成).*(?:度|档|挡|[%％])|(?:风量|温度|空调).*(?:度|档|挡|打开|关闭))"));
    int count = 0;
    for (const auto &clause : normalized(request).split(separator))
        if (command.match(clause).hasMatch() && ++count == 2)
            return true;
    return false;
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
    static const QRegularExpression explicitTarget(QStringLiteral(
        "^(?:请|帮我)?(?:(主驾|副驾)(?:的)?(?:空调)?(?:温度)?|"
        "(左前|右前|左后|右后)(?:的)?(?:车窗|窗户|窗))(?:再)?"
        "(?:调)?(高|低|升高|降低|升|降|开|关)(?:一?点|([0-9]{1,2}(?:\\.[05])?|[零一二两三四五六七八九十]{1,3}|半)(度|[%％]|个百分点))$"));
    const auto targeted = explicitTarget.match(phrase);
    if (targeted.hasMatch()) {
        const bool temperature = !targeted.captured(1).isEmpty();
        const auto action = targeted.captured(3);
        const bool increase = action == QStringLiteral("高") || action == QStringLiteral("升高")
            || action == QStringLiteral("升") || action == QStringLiteral("开");
        if ((temperature && (action == QStringLiteral("开") || action == QStringLiteral("关"))) ||
            (!temperature && action != QStringLiteral("开") && action != QStringLiteral("关")))
            return {};
        const auto unit = targeted.captured(5);
        if (!unit.isEmpty() && (temperature != (unit == QStringLiteral("度"))))
            return {};
        const auto number = targeted.captured(4);
        bool numeric = false;
        double amount = number.toDouble(&numeric);
        if (number.isEmpty())
            amount = temperature ? 0.5 : 10;
        else if (!numeric)
            amount = number == QStringLiteral("半") ? 0.5 : assistantCommandInteger(number);
        if (amount < 0)
            return {};
        return AssistantVehicleSetting{temperature ? Kind::Temperature : Kind::Window,
            int((temperature ? ClimateZones : WindowPositions).indexOf(targeted.captured(temperature ? 1 : 2))),
            increase ? amount : -amount, true};
    }
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
        "(?:^|[，,。；;])(?:还是|先)?(?:取消(?:这次)?|算了|不用了|不调了|不做了|不确认|结束对话|停止聆听)$"));
    return cancel.match(normalized(request)).hasMatch();
}
