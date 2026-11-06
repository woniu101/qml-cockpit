#include "AssistantVehicleQuery.h"
#include "VehicleState.h"

#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>

namespace {
const QStringList names{QStringLiteral("左前"), QStringLiteral("右前"),
                        QStringLiteral("左后"), QStringLiteral("右后")};
const QStringList modelTargets{"leftFront", "rightFront", "leftRear", "rightRear", "all"};

int windowTargetMentioned(const QString &request) {
    static const QRegularExpression target(QStringLiteral(
        "左前|右前|左后|右后|副驾驶(?:位)?|副驾|主驾驶(?:位)?|驾驶位|主驾|全部|全车|所有|四个|四扇|四窗"));
    auto matches = target.globalMatch(request);
    int selected = -1;
    while (matches.hasNext()) {
        const auto word = matches.next().captured();
        int index = int(names.indexOf(word));
        if (index < 0)
            index = word.startsWith(QStringLiteral("副驾")) ? 1
                : word.contains(QChar(u'驾')) ? 0 : 4;
        if (selected >= 0 && selected != index)
            return -1;
        selected = index;
    }
    return selected;
}
}

QString assistantModelVehicleAnswer(const VehicleState &vehicle, const QString &request, const QJsonObject &result) {
    const auto value = result.value("proposal");
    if (result.value("speech_act").toString() != "QUERY" || !value.isObject() ||
        result.value("confidence").toDouble() < 0.80)
        return {};

    static const QRegularExpression nonQuery(QStringLiteral(
        "别|不要|不用|不必|不想|不需要|没有让|没让|没叫|没说|只是念|只是举例|这句话|这几个字|引用|翻译|假设|如果|"
        "然后|同时|顺便|并(?:且|将|把|帮)|再把|(?:开到|调到|降到|设为|设置为|改成)(?:百分之|[0-9零一二三四五六七八九十])"));
    if (nonQuery.match(request).hasMatch())
        return {};

    const auto intent = result.value("intent").toString();
    const auto proposal = value.toObject();
    QString answer;
    if (intent == "BATTERY_QUERY" && proposal.isEmpty()) {
        static const QRegularExpression battery(QStringLiteral(
            "电量|电池|剩.{0,7}电|还有.{0,7}电|电.{0,5}剩|(?<![a-z0-9_])soc(?![a-z0-9_])"),
            QRegularExpression::CaseInsensitiveOption);
        static const QRegularExpression asking(QStringLiteral("多少|几成|百分之几|百分比|查|看|报|读|显示|告诉|知道|什么数值"));
        static const QRegularExpression otherMetric(QStringLiteral(
            "寿命|健康|温度|电压|电流|功率|容量|衰减|续航|里程|充电|滤芯|手机|电脑|钥匙|车窗|后视镜"));
        if (battery.match(request).hasMatch() && asking.match(request).hasMatch() && !otherMetric.match(request).hasMatch())
            answer = assistantBatteryAnswer(vehicle);
    } else if (intent == "WINDOW_QUERY" && proposal.size() == 1) {
        static const QRegularExpression asking(QStringLiteral(
            "开度|开了?多少|开了?多大|打开.{0,3}百分比|开着(?:没|吗)|"
            "关(?:严|好|紧|上)(?:了)?(?:没|吗)|有没有.{0,4}(?:留缝|关|开)|"
            "是否.{0,4}(?:关|开)|是不是.{0,4}(?:关|开)|开合状态|打开情况|车窗状态|(?:车)?窗(?:户)?的状态"));
        const int index = int(modelTargets.indexOf(proposal.value("target").toString()));
        if (index >= 0 && request.contains(QChar(u'窗')) && !request.contains(QStringLiteral("电量")) &&
            asking.match(request).hasMatch() && windowTargetMentioned(request) == index)
            answer = assistantWindowAnswer(vehicle, index == 4 ? -1 : index);
    }
    const auto ruleAnswer = assistantVehicleAnswer(vehicle, request);
    return ruleAnswer.isEmpty() || ruleAnswer == answer ? answer : QString();
}

QString assistantBatteryAnswer(const VehicleState &vehicle) {
    if (!vehicle.energyReady() || !vehicle.batteryKnown())
        return QStringLiteral("当前电量数据不可用，请检查数据来源");
    return QStringLiteral("当前电量 %1%，来源为%2").arg(qRound(vehicle.batteryPercent())).arg(vehicle.energySourceText());
}

QString assistantVehicleAnswer(const VehicleState &vehicle, const QString &request) {
    static const QRegularExpression battery(QStringLiteral(
        "^(?:请问|查询|查看|告诉我)?(?:现在|当前)?(?:车辆|汽车|电池)?(?:电量(?:还剩多少|是多少|多少)?|还剩多少电|还有多少电|剩余多少电)$"));
    if (battery.match(request).hasMatch())
        return assistantBatteryAnswer(vehicle);

    static const QRegularExpression temperature(QStringLiteral(
        "^(?:请问|查询|查看|告诉我)?(?:现在|当前)?(主驾|副驾|主驾驶|副驾驶)?(?:的)?(?:空调)?(?:设定)?(?:温度(?:是多少|多少|几度)?|现在多少度|多少度)$"));
    const auto zone = temperature.match(request);
    if (zone.hasMatch()) {
        const auto driver = QString::number(vehicle.driverTemperature(), 'f', 1);
        const auto passenger = QString::number(vehicle.passengerTemperature(), 'f', 1);
        if (zone.captured(1).startsWith(QStringLiteral("主驾")))
            return QStringLiteral("主驾空调设定温度 %1 度").arg(driver);
        if (zone.captured(1).startsWith(QStringLiteral("副驾")))
            return QStringLiteral("副驾空调设定温度 %1 度").arg(passenger);
        return QStringLiteral("主驾设定 %1 度，副驾设定 %2 度").arg(driver, passenger);
    }

    static const QRegularExpression window(QStringLiteral(
        "^(?:请问|查询|查看|告诉我)?(?:现在|当前)?(左前|右前|左后|右后|全部|全车)?车窗(?:现在|当前)?(?:实际)?(?:开度(?:是多少|多少)?|状态|开了多少|关上了吗|关好了吗)$"));
    const auto position = window.match(request);
    if (!position.hasMatch())
        return {};
    return assistantWindowAnswer(vehicle, int(names.indexOf(position.captured(1))));
}

QString assistantWindowAnswer(const VehicleState &vehicle, int selected) {
    if (!vehicle.externalReady())
        return QStringLiteral("外部车辆数据暂不可用，无法查询车窗开度");
    const auto openings = vehicle.windowActual();
    QStringList result;
    for (int i = 0; i < names.size() && i < openings.size(); ++i) {
        if (selected >= 0 && selected != i)
            continue;
        const int percent = qRound(openings.at(i).toDouble());
        result << (percent == 0 ? QStringLiteral("%1车窗已关闭").arg(names.at(i))
                               : QStringLiteral("%1车窗开度 %2%").arg(names.at(i)).arg(percent));
    }
    return result.isEmpty() ? QStringLiteral("当前车窗数据不可用") : result.join(QStringLiteral("，"));
}
