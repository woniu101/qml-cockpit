#include "AssistantVehicleQuery.h"
#include "VehicleState.h"

#include <QRegularExpression>
#include <QStringList>

namespace {
const QStringList names{QStringLiteral("左前"), QStringLiteral("右前"),
                        QStringLiteral("左后"), QStringLiteral("右后")};
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
