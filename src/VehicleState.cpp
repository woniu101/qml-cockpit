#include "VehicleState.h"
#include <algorithm>
#include <cmath>
#include <QUrl>
#include <QFileInfo>
#include <QRandomGenerator>

VehicleState::VehicleState(QObject *parent,SystemStatus *system):QObject(parent) {
    m_link=new VehicleLink(this);
    m_system=system?system:new SystemStatus(this);
    connect(m_system,&SystemStatus::powerChanged,this,&VehicleState::applySystemPower);
    connect(m_link,&VehicleLink::snapshotReceived,this,&VehicleState::applyExternal);
    connect(m_link,&VehicleLink::commandResult,this,&VehicleState::externalResult);
    connect(m_link,&VehicleLink::statusChanged,this,[this]{if(!m_link->ready())m_remoteDragging.fill(false);emit sourceChanged();emit windowProgressChanged();});
    m_route=makeRoute(m_destination);
    for(auto &message:m_windowMessages)message=QStringLiteral("拖动滑条，或选择常用开度");
    connect(this,&VehicleState::changed,this,&VehicleState::statusChanged);
    connect(this,&VehicleState::motionsChanged,this,&VehicleState::statusChanged);
    m_windowTimer.setInterval(16);m_windowTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_windowTimer,&QTimer::timeout,this,&VehicleState::tickWindows);
    m_saveTimer.setSingleShot(true);m_saveTimer.setInterval(350);
    connect(&m_saveTimer,&QTimer::timeout,this,&VehicleState::savePreferences);
    loadPreferences();
}
VehicleState::~VehicleState(){savePreferences();}
void VehicleState::setDriverTemperature(double value) {
    if(!std::isfinite(value))return;
    value=std::clamp(std::round(value*2)/2,16.0,30.0);
    if(qFuzzyCompare(value,m_driver))return;
    m_driver=value;if(m_sync)m_passenger=value;emit changed();schedulePreferences();
}
void VehicleState::setPassengerTemperature(double value) {
    if(!std::isfinite(value))return;
    value=std::clamp(std::round(value*2)/2,16.0,30.0);
    if(qFuzzyCompare(value,m_passenger))return;
    m_passenger=value;if(m_sync)m_driver=value;emit changed();schedulePreferences();
}
void VehicleState::setFan(int value) { value=std::clamp(value,0,7);if(m_fan==value)return;m_fan=value;m_auto=false;emit changed();schedulePreferences(); }
void VehicleState::setAcOn(bool value) { if(m_ac==value)return;m_ac=value;emit changed();schedulePreferences(); }
void VehicleState::setAutomatic(bool value) { if(m_auto==value)return;m_auto=value;if(value){m_ac=true;if(!m_fan)m_fan=3;}emit changed();schedulePreferences(); }
void VehicleState::setClimateSync(bool value) { if(m_sync==value)return;m_sync=value;if(value)m_passenger=m_driver;emit changed();schedulePreferences(); }
void VehicleState::setAirDirection(const QString &value) { if(!QStringList{"face","feet","mixed"}.contains(value)||value==m_airDirection)return;m_airDirection=value;emit changed();schedulePreferences(); }
void VehicleState::setDefrost(bool value) {
    if(m_defrost==value)return;
    if(value){m_defrostFan=m_fan;m_defrostAuto=m_auto;m_defrostAc=m_ac;m_fan=5;m_auto=false;m_ac=true;}
    else {m_fan=m_defrostFan;m_auto=m_defrostAuto;m_ac=m_defrostAc;}
    m_defrost=value;emit changed();
}
void VehicleState::setHeatLevel(int value) {value=std::clamp(value,0,3);if(value==m_heat)return;m_heat=value;emit changed();schedulePreferences();}
void VehicleState::setSeatHeat(bool value) {setHeatLevel(value?std::max(1,m_heat):0);}
void VehicleState::setLightMode(const QString &value) {if(!QStringList{"off","auto","on"}.contains(value)||value==m_lightMode)return;m_lightMode=value;emit changed();schedulePreferences();}
void VehicleState::setLightsOn(bool value) {setLightMode(value?"on":"off");}
void VehicleState::setHighBeam(bool value) {if(value==m_highBeam)return;m_highBeam=value;if(value)m_lightMode="on";emit changed();}
void VehicleState::setFogLights(bool value) {if(value==m_fog)return;m_fog=value;if(value && m_lightMode=="off")m_lightMode="on";emit changed();}
void VehicleState::setScene(const QString &value) {
    if(!QStringList{"comfort","rest","camp"}.contains(value)||value==m_scene)return;
    if(!parked() && value!="comfort"){emit toast(QStringLiteral("请挂 P 挡后使用场景模式"),"warning");return;}
    if(m_scene=="comfort"){
        m_savedDriver=m_driver;m_savedPassenger=m_passenger;m_savedFan=m_fan;m_savedAc=m_ac;m_savedAuto=m_auto;
        m_savedSync=m_sync;m_savedDefrost=m_defrost;m_savedHeat=m_heat;m_savedAirDirection=m_airDirection;
        m_savedDefrostFan=m_defrostFan;m_savedDefrostAuto=m_defrostAuto;m_savedDefrostAc=m_defrostAc;
    }
    m_scene=value;
    if(value=="comfort"){
        m_driver=m_savedDriver;m_passenger=m_savedPassenger;m_fan=m_savedFan;m_ac=m_savedAc;m_auto=m_savedAuto;
        m_sync=m_savedSync;m_defrost=m_savedDefrost;m_heat=m_savedHeat;m_airDirection=m_savedAirDirection;
        m_defrostFan=m_savedDefrostFan;m_defrostAuto=m_savedDefrostAuto;m_defrostAc=m_savedDefrostAc;
    }
    else {m_driver=m_passenger=value=="rest"?23:24;m_fan=2;m_ac=true;m_auto=true;m_defrost=false;}
    emit changed();
    emit toast(value=="comfort"?QStringLiteral("已恢复进入场景前的空调设置"):value=="rest"?QStringLiteral("小憩模式已开启"):QStringLiteral("露营模式已开启"));
}
