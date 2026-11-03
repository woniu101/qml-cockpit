#include "VehicleState.h"
#include <algorithm>
#include <cmath>

VehicleState::Route VehicleState::makeRoute(const QString &destination) {
    Route result;result.name=destination.trimmed().left(32);
    auto points=[&](std::initializer_list<QPointF> items){for(const auto &p:items)result.points.append(QVariantMap{{"x",p.x()},{"y",p.y()}});};
    if(destination==QStringLiteral("回家")){
        result.km=14;result.minutes=28;points({{899,661},{899,425},{678,425},{678,243},{1183,243}});
        result.streets={QStringLiteral("望江路"),QStringLiteral("林荫路"),QStringLiteral("云杉路"),result.name};result.turns={"left","right","right","finish"};
    }else if(destination==QStringLiteral("公司")){
        result.km=9.6;result.minutes=18;points({{899,661},{899,425},{1262,425},{1262,243}});
        result.streets={QStringLiteral("望江路"),QStringLiteral("滨江大道"),result.name};result.turns={"right","left","finish"};
    }else if(destination.contains(QStringLiteral("公园"))){
        result.km=6.8;result.minutes=14;points({{899,661},{678,661},{678,425},{552,425}});
        result.streets={QStringLiteral("栖霞路"),QStringLiteral("林荫路"),result.name};result.turns={"right","left","finish"};
    }else if(destination.contains(QStringLiteral("充电"))){
        result.km=destination.contains(QStringLiteral("云杉"))?2.4:destination.contains(QStringLiteral("望江"))?3.1:1.2;result.minutes=int(std::ceil(result.km*3));
        points({{899,661},{1098,661},{1098,542}});result.streets={QStringLiteral("栖霞路"),result.name};result.turns={"left","finish"};
    }else{
        result.km=12;result.minutes=22;points({{899,661},{899,425},{1098,425},{1098,243},{1262,243}});
        result.streets={QStringLiteral("望江路"),QStringLiteral("云杉东路"),QStringLiteral("滨江大道"),result.name};result.turns={"right","left","right","finish"};
    }
    return result;
}
QVariantMap VehicleState::routePreview(const QString &destination) const {
    const auto route=makeRoute(destination);return {{"km",route.km},{"minutes",route.minutes}};
}
VehicleState::Location VehicleState::routeLocation() const {
    Location value;if(m_route.points.size()<2)return value;
    auto point=[&](int i){const auto p=m_route.points[i].toMap();return QPointF(p["x"].toDouble(),p["y"].toDouble());};
    QList<double> lengths;value.total=0;
    for(int i=0;i<m_route.points.size()-1;++i){const auto d=point(i+1)-point(i);const double length=std::hypot(d.x(),d.y());lengths<<length;value.total+=length;}
    double remaining=std::clamp(m_progress,0.0,1.0)*value.total;
    for(int i=0;i<lengths.size();++i){
        if(remaining<=lengths[i] || i==lengths.size()-1){
            const auto a=point(i),d=point(i+1)-a;const double f=lengths[i]>0?std::clamp(remaining/lengths[i],0.0,1.0):0;
            value.point=a+d*f;value.heading=std::atan2(d.x(),-d.y())*180.0/3.141592653589793;
            value.remaining=std::max(0.0,lengths[i]-remaining);value.leg=i;return value;
        }
        remaining-=lengths[i];
    }
    return value;
}
QPointF VehicleState::routeMarker() const {return routeLocation().point;}
double VehicleState::routeHeading() const {return routeLocation().heading;}
int VehicleState::turnDistance() const {const auto p=routeLocation();return std::max(0,int(std::round(p.remaining/p.total*m_route.km*1000/10))*10);}
QString VehicleState::turnType() const {return m_route.turns.value(routeLocation().leg,"finish");}
QString VehicleState::turnStreet() const {return m_route.streets.value(routeLocation().leg,m_destination);}
QString VehicleState::laneHint() const {return turnType()=="left"?QStringLiteral("靠左行驶，准备左转"):turnType()=="right"?QStringLiteral("靠右行驶，准备右转"):QStringLiteral("即将到达，请注意目的地入口");}
double VehicleState::remainingKm() const {return m_route.km*(1.0-m_progress);}
int VehicleState::remainingMinutes() const {
    if(!m_navigating)return 0;
    const double minutes=m_speed>0?remainingKm()/m_speed*60:m_route.minutes*(1-m_progress);
    return std::max(1,int(std::ceil(minutes)));
}
bool VehicleState::startNavigation(const QString &destination) {
    if(destination.trimmed().isEmpty()){emit toast(QStringLiteral("请输入目的地"),"warning");return false;}
    m_destination=destination.trimmed().left(32);m_route=makeRoute(m_destination);m_progress=0;m_navigating=true;
    emit changed();emit toast(parked()?QStringLiteral("路线已规划，车辆保持驻车"):QStringLiteral("已开始示意导航"));return true;
}
void VehicleState::endNavigation() {m_navigating=false;emit changed();emit toast(QStringLiteral("导航已结束，挡位与车速保持不变"));}
bool VehicleState::canDrive() {
    if(!energyReady()){emit toast(QStringLiteral("电量或供电状态待确认，请等待刷新或切换数据来源"),"warning");return false;}
    if(!m_motions.isEmpty()){emit toast(QStringLiteral("请等待车门与备厢开合完成"),"warning");return false;}
    if(m_hatch||m_hood||m_port||m_plugged||std::any_of(m_doors.begin(),m_doors.end(),[](bool v){return v;})){
        emit toast(QStringLiteral("请关闭车门、前后备厢和充电口，并断开充电枪"),"warning");return false;
    }
    if(m_battery<=0){emit toast(QStringLiteral("演示电量不足，请先充电"),"warning");return false;}return true;
}
void VehicleState::setGear(const QString &value) {
    if(externalCommand("setGear",{{"value",value}},QStringLiteral("切换挡位")))return;
    if(!QStringList{"P","R","N","D"}.contains(value)||value==m_gear)return;
    if(value!="P"&&(m_plugged||m_charging)){emit toast(QStringLiteral("请暂停充电并断开充电枪后换挡"),"warning");return;}
    if((value=="D"||value=="R")&&!canDrive())return;
    if(m_scene!="comfort" && (value=="D"||value=="R"))setScene("comfort");
    m_gear=value;if(value=="P"||value=="N")m_speed=0;
    if(value=="R")m_speed=std::min(m_speed,15);
    if(value=="D"||value=="R"){m_locked=true;m_mirrors=false;}
    emit changed();
}
void VehicleState::setSpeed(int value) {
    if(externalCommand("setSpeed",{{"value",value}},QStringLiteral("调整车速")))return;
    value=std::clamp(value,0,m_gear=="R"?15:100);
    if((m_gear!="D"&&m_gear!="R") && value>0)return;
    if(value>0&&systemMode()&&!canDrive())return;
    if(value==m_speed)return;m_speed=value;emit changed();
}
void VehicleState::startDriving() {if(externalCommand("startDriving",{},QStringLiteral("开始行驶")))return;if(!canDrive())return;setGear("D");if(m_gear=="D"){setSpeed(40);emit toast(QStringLiteral("行驶演示已开始 · D 挡 40 km/h"));}}
void VehicleState::parkVehicle() {if(externalCommand("park",{},QStringLiteral("挂 P 驻车")))return;m_speed=0;m_gear="P";emit changed();emit toast(QStringLiteral("车辆已驻车，导航可继续保留"));}
void VehicleState::advanceRoute(double seconds) {
    if(externalMode())return;
    if(!m_navigating || m_speed<=0 || m_gear!="D" || !std::isfinite(seconds) || seconds<=0)return;
    m_progress=std::min(1.0,m_progress+m_speed*seconds/3600/m_route.km);
    if(m_progress>=1){m_navigating=false;emit toast(QStringLiteral("已到达目的地，导航结束"));}emit changed();
}
int VehicleState::rangeKm() const {return qRound(m_battery*5.2);}
QString VehicleState::chargeStatus() const {
    if(systemMode()&&!m_systemFallback)return m_system->power()["statusText"].toString();
    if(!m_plugged)return QStringLiteral("未连接充电枪");
    if(m_charging)return QStringLiteral("正在充电");
    return m_battery>=m_chargeTarget?QStringLiteral("已达到目标电量"):QStringLiteral("已连接 · 等待充电");
}
int VehicleState::chargeMinutes() const {return chargePowerKnown()&&m_plugged?std::max(0,int(std::ceil((m_chargeTarget-m_battery)/100*60/55*60))):0;}
void VehicleState::setChargeTarget(int value) {if(systemEnergyCommand())return;if(externalCommand("setChargeTarget",{{"value",value}},QStringLiteral("充电目标")))return;value=std::clamp(value,50,100);if(value==m_chargeTarget)return;m_chargeTarget=value;if(m_battery>=value)m_charging=false;emit changed();schedulePreferences();}
void VehicleState::togglePlug() {
    if(systemEnergyCommand())return;
    if(externalCommand("setPlugged",{{"value",!m_plugged}},QStringLiteral("充电枪连接")))return;
    if(!parked()||!m_port){emit toast(QStringLiteral("请驻车并打开充电口后连接充电枪"),"warning");return;}
    if(m_charging){emit toast(QStringLiteral("请先暂停充电，再断开充电枪"),"warning");return;}
    m_plugged=!m_plugged;emit changed();
}
void VehicleState::startCharging() {
    if(systemEnergyCommand())return;
    if(externalCommand("startCharging",{},QStringLiteral("开始充电")))return;
    if(!parked()||!m_plugged||!m_port){emit toast(QStringLiteral("请驻车并连接充电枪"),"warning");return;}
    if(m_battery>=m_chargeTarget){emit toast(QStringLiteral("已达到目标电量，请提高充电目标"),"info");return;}
    m_charging=true;emit changed();emit toast(QStringLiteral("补能演示已开始"));
}
void VehicleState::stopCharging() {if(systemEnergyCommand())return;if(externalCommand("stopCharging",{},QStringLiteral("暂停充电")))return;if(!m_charging)return;m_charging=false;emit changed();}
void VehicleState::advanceSimulation(double seconds) {
    if(externalMode()){refreshTheme();if(seconds>1)externalCommand("advance",{{"seconds",seconds}},QStringLiteral("模拟快进"));return;}
    if(!std::isfinite(seconds)||seconds<=0)return;seconds=std::min(seconds,3600.0);refreshTheme();
    advanceRoute(seconds);bool energyChanged=false;
    if(systemMode()&&!m_systemFallback){if(m_speed>0){m_tripKm+=m_speed*seconds/3600;emit changed();}return;}
    if(m_speed>0){m_tripKm+=m_speed*seconds/3600;m_battery=std::max(0.0,m_battery-m_speed*seconds/3600*0.15/60*100);energyChanged=true;if(m_battery<=0)m_speed=0;}
    if(m_charging){
        m_battery=std::min(double(m_chargeTarget),m_battery+55.0/60/3600*100*seconds);energyChanged=true;
        if(m_battery>=m_chargeTarget){m_charging=false;emit toast(QStringLiteral("充电完成，已达到 %1%").arg(m_chargeTarget));}
    }
    if(energyChanged)emit changed();
}
