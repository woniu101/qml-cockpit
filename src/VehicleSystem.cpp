#include "VehicleState.h"

bool VehicleState::energyReady() const {
    return systemMode()?m_systemFallback||m_system->powerReading().valid:externalReady();
}
bool VehicleState::batteryKnown() const {return !systemMode()||m_systemFallback||m_systemHadSoc;}
QString VehicleState::energySourceText() const {
    if(externalMode())return QStringLiteral("外部模拟器");
    if(!systemMode())return QStringLiteral("内置演示");
    if(m_systemFallback)return m_system->powerReading().readOk?QStringLiteral("无系统电池 · 模拟电量"):QStringLiteral("电源状态未知 · 继续模拟电量");
    return energyReady()?QStringLiteral("Windows 实际电量"):QStringLiteral("电脑映射 · 电源状态待确认");
}
QVariantMap VehicleState::energySnapshot() const {return {{"battery",m_battery},{"plugged",m_plugged},{"charging",m_charging},{"port",m_port},{"chargeTarget",m_chargeTarget}};}
void VehicleState::restoreEnergy(const QVariantMap &s){if(s.isEmpty())return;m_battery=s["battery"].toDouble();m_plugged=s["plugged"].toBool();m_charging=s["charging"].toBool();m_port=s["port"].toBool();m_chargeTarget=s["chargeTarget"].toInt();}
bool VehicleState::systemEnergyCommand(){
    if(chargeControlAvailable())return false;
    emit toast(QStringLiteral("当前使用 Windows 实际电源状态；模拟补能请切换数据来源"),"warning");return true;
}
void VehicleState::applySystemPower(){
    if(!systemMode())return;
    const auto &p=m_system->powerReading();
    if(p.readOk&&p.presenceKnown&&!p.hasBattery){
        if(!m_systemFallback){
            restoreEnergy(m_energyBeforeSystem);m_systemFallback=true;
            if(m_speed>0){m_speed=0;emit toast(QStringLiteral("系统电池已移除，已改用模拟电量并暂停行驶演示"),"warning");}
        }
    }else if(p.readOk&&p.presenceKnown&&p.hasBattery){
        if(m_systemFallback){m_energyBeforeSystem=energySnapshot();m_plugged=m_charging=m_port=false;m_systemFallback=false;m_systemHadSoc=false;}
        if(p.socKnown){m_battery=p.percent;m_systemHadSoc=true;}
        if(p.valid){
            const bool newConnection=p.onAc&&!m_plugged;
            m_plugged=p.onAc;m_port=p.onAc;m_charging=p.charging;
            if(newConnection&&!parked()){
                m_speed=0;emit toast(QStringLiteral("检测到外部供电，行驶演示已暂停，请挂 P 挡查看补能状态"),"warning");
            }
            if(m_battery<=0)m_speed=0;
        }
    }
    // Read failures keep the last confirmed values. They never start a new fallback.
    if(!energyReady()&&m_speed>0){m_speed=0;emit toast(QStringLiteral("电源状态待确认，行驶演示已暂停"),"warning");}
    emit sourceChanged();emit changed();
}
