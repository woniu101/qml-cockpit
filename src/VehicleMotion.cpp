#include "VehicleState.h"
#include <algorithm>
#include <cmath>

namespace {const QStringList names={QStringLiteral("左前"),QStringLiteral("右前"),QStringLiteral("左后"),QStringLiteral("右后")};}
QVariantList VehicleState::doors() const {QVariantList result;for(bool value:m_doors)result.append(value);return result;}
QVariantList VehicleState::windows() const {QVariantList result;for(double value:m_windowActual)result.append(qRound(value));return result;}
QVariantList VehicleState::windowActual() const {QVariantList result;for(double value:m_windowActual)result.append(value);return result;}
QVariantList VehicleState::windowTargets() const {QVariantList result;for(int i=0;i<4;++i)result.append(externalMode()&&m_remoteDragging[i]?m_remoteDragTargets[i]:m_windowTargets[i]);return result;}
QVariantList VehicleState::windowMessages() const {QVariantList result;for(const auto &value:m_windowMessages)result.append(value);return result;}
QVariantList VehicleState::windowBusy() const {QVariantList result;for(int i=0;i<4;++i)result.append(m_windowMoving[i]||m_windowDragging[i]||(externalMode()&&m_remoteDragging[i]));return result;}
QVariantList VehicleState::doorBusy() const {QVariantList result;for(int i=0;i<4;++i)result.append(m_motions.contains(QString("door%1").arg(i)));return result;}
bool VehicleState::busy() const {return !m_motions.isEmpty() || std::any_of(m_windowMoving.begin(),m_windowMoving.end(),[](bool v){return v;}) || std::any_of(m_windowDragging.begin(),m_windowDragging.end(),[](bool v){return v;});}
void VehicleState::beginMotion(const QString &key,const QString &label,const QString &completion,int duration) {
    if(m_motions.isEmpty())m_motionCompletion=completion;else if(m_motionCompletion!=completion)m_motionCompletion=QStringLiteral("车辆部件调整完成");
    const auto serial=++m_motionSerial;m_motions.insert(key,{serial,label});emit motionsChanged();
    QTimer::singleShot(duration,this,[this,key,serial]{
        auto found=m_motions.constFind(key);if(found==m_motions.cend()||found->serial!=serial)return;
        m_motions.remove(key);emit motionsChanged();
        if(m_motions.isEmpty()){const auto message=m_motionCompletion;m_motionCompletion.clear();emit toast(message);}
    });
}
QString VehicleState::operationSummary() const {
    int count=m_motions.size();for(int i=0;i<4;++i)if(m_windowMoving[i]||m_windowDragging[i])++count;
    return count?QStringLiteral("正在调整 %1 个车辆部件…").arg(count):QString();
}
QString VehicleState::globalProgress() const {
    if(!m_windowBatch.isEmpty())return m_windowBatch=="close"?QStringLiteral("正在关闭全部车窗…"):QStringLiteral("正在调整全车通风开度…");
    return m_motions.isEmpty()?QString():operationSummary();
}
void VehicleState::setWindowInternal(int index,int opening,bool batch) {
    if(index<0||index>=4)return;opening=std::clamp(opening,0,100);
    if(!batch)m_windowBatch.clear();
    m_windowDragging[index]=false;
    if(std::abs(m_windowActual[index]-opening)<0.01){m_windowTargets[index]=opening;m_windowMoving[index]=false;emit windowProgressChanged();emit motionsChanged();return;}
    m_windowTargets[index]=opening;m_windowMoving[index]=true;
    m_windowMessages[index]=m_windowActual[index]<opening?QStringLiteral("正在降窗"):QStringLiteral("正在升窗");
    if(!m_windowTimer.isActive()){m_windowClock.start();m_windowTimer.start();}
    emit windowProgressChanged();emit motionsChanged();
}
void VehicleState::setWindow(int index,int opening) {if(externalCommand("setWindow",{{"index",index},{"target",opening}},QStringLiteral("调整车窗")))return;setWindowInternal(index,opening,false);}
void VehicleState::tickWindows() {
    if(externalMode())return;
    const double step=std::min(0.1,m_windowClock.restart()/1000.0)*100.0/1.8;
    bool finished=false,active=false;
    for(int i=0;i<4;++i){
        if(!m_windowMoving[i])continue;
        double &actual=m_windowActual[i];const double target=m_windowTargets[i];
        if(std::abs(target-actual)<=step){actual=target;m_windowMoving[i]=false;finished=true;m_windowMessages[i]=QStringLiteral("%1车窗已调至 %2%").arg(names[i]).arg(qRound(actual));}
        else {actual+=actual<target?step:-step;active=true;}
    }
    emit windowProgressChanged();
    if(finished){emit motionsChanged();emit statusChanged();}
    else if(++m_windowNotifyTicks%8==0)emit statusChanged();
    if(!active){
        m_windowTimer.stop();
        if(!m_windowBatch.isEmpty()){
            const bool close=m_windowBatch=="close";m_windowBatch.clear();
            emit toast(close?QStringLiteral("全部车窗已关闭"):QStringLiteral("全车车窗已调至通风 15%"));emit motionsChanged();
        }
    }
}
void VehicleState::beginWindowDrag(int index) {
    if(externalMode()){if(index<0||index>=4||!externalReady())return;m_remoteDragging[index]=true;m_remoteDragTargets[index]=m_windowTargets[index];m_windowMessages[index]=QStringLiteral("预览目标，松手后发送至模拟器");emit windowProgressChanged();return;}
    if(index<0||index>=4)return;m_windowBatch.clear();m_windowMoving[index]=false;m_windowDragging[index]=true;
    m_windowTargets[index]=qRound(m_windowActual[index]);m_windowMessages[index]=QStringLiteral("拖动预览开度，松手确认");emit windowProgressChanged();emit motionsChanged();
}
void VehicleState::previewWindow(int index,int opening) {
    if(externalMode()){if(index<0||index>=4||!m_remoteDragging[index])return;m_remoteDragTargets[index]=std::clamp(opening,0,100);emit windowProgressChanged();return;}
    if(index<0||index>=4||!m_windowDragging[index])return;
    m_windowActual[index]=m_windowTargets[index]=std::clamp(opening,0,100);emit windowProgressChanged();
}
void VehicleState::endWindowDrag(int index) {
    if(externalMode()){if(index<0||index>=4||!m_remoteDragging[index])return;const int target=m_remoteDragTargets[index];m_remoteDragging[index]=false;setWindow(index,target);emit windowProgressChanged();return;}
    if(index<0||index>=4||!m_windowDragging[index])return;m_windowDragging[index]=false;
    m_windowMessages[index]=QStringLiteral("%1车窗已调至 %2%").arg(names[index]).arg(qRound(m_windowActual[index]));
    emit windowProgressChanged();emit motionsChanged();
}
void VehicleState::stopWindow(int index) {
    if(externalCommand("stopWindow",{{"index",index}},QStringLiteral("停止车窗")))return;
    if(index<0||index>=4)return;m_windowBatch.clear();m_windowMoving[index]=m_windowDragging[index]=false;
    m_windowActual[index]=m_windowTargets[index]=qRound(m_windowActual[index]);
    m_windowMessages[index]=QStringLiteral("已停在 %1%").arg(m_windowTargets[index]);emit windowProgressChanged();emit motionsChanged();
}
void VehicleState::closeWindows() {
    if(externalCommand("closeWindows",{},QStringLiteral("全部关窗")))return;
    bool any=false;for(int i=0;i<4;++i)if(m_windowActual[i]>0.01 || m_windowMoving[i] || m_windowDragging[i]){setWindowInternal(i,0,true);any|=m_windowMoving[i];}
    m_windowBatch=any?"close":"";emit motionsChanged();
}
void VehicleState::ventilateWindows() {
    if(externalCommand("ventilateWindows",{},QStringLiteral("全车通风")))return;
    bool any=false;for(int i=0;i<4;++i){setWindowInternal(i,15,true);any|=m_windowMoving[i];}
    m_windowBatch=any?"vent":"";emit motionsChanged();
}
void VehicleState::setDoor(int index,bool open) {
    if(externalCommand("setDoor",{{"index",index},{"open",open}},QStringLiteral("车门操作")))return;
    if(index<0||index>=4||m_doors[index]==open)return;
    if(!parked()){emit toast(QStringLiteral("请挂 P 挡后操作车门"),"warning");return;}
    if(open)m_locked=false;m_doors[index]=open;const auto name=names[index]+QStringLiteral("车门");
    beginMotion(QString("door%1").arg(index),name+(open?QStringLiteral("正在打开"):QStringLiteral("正在关闭")),name+(open?QStringLiteral("已打开"):QStringLiteral("已关闭")),doorMotionDuration());emit changed();
}
void VehicleState::closeDoors() {
    if(externalCommand("closeDoors",{},QStringLiteral("关闭全部车门")))return;
    bool any=false;for(int i=0;i<4;++i)if(m_doors[i]){m_doors[i]=false;any=true;beginMotion(QString("door%1").arg(i),QStringLiteral("正在关闭全部车门"),QStringLiteral("全部车门已关闭"),doorMotionDuration());}
    if(any)emit changed();
}
void VehicleState::toggleLock() {
    if(externalCommand("setLocked",{{"value",!m_locked}},QStringLiteral("车门锁")))return;
    if(!parked()){emit toast(QStringLiteral("行驶演示中保持车门锁定"),"warning");return;}
    bool moving=false;for(int i=0;i<4;++i)moving|=m_motions.contains(QString("door%1").arg(i));
    if(!m_locked && (moving || std::any_of(m_doors.begin(),m_doors.end(),[](bool v){return v;}))){emit toast(QStringLiteral("请等待全部车门关闭后锁车"),"warning");return;}
    m_locked=!m_locked;emit changed();emit toast(m_locked?QStringLiteral("车辆已锁定"):QStringLiteral("车辆已解锁"));
}
void VehicleState::toggleHatch() {
    if(externalCommand("setHatch",{{"value",!m_hatch}},QStringLiteral("后备厢")))return;
    if(!parked()){emit toast(QStringLiteral("请挂 P 挡后操作后备厢"),"warning");return;}
    m_hatch=!m_hatch;beginMotion("hatch",QStringLiteral("后备厢正在开合"),m_hatch?QStringLiteral("后备厢已打开"):QStringLiteral("后备厢已关闭"),hatchMotionDuration());emit changed();
}
void VehicleState::toggleHood() {
    if(externalCommand("setHood",{{"value",!m_hood}},QStringLiteral("前备厢")))return;
    if(!parked()){emit toast(QStringLiteral("请挂 P 挡后操作前备厢"),"warning");return;}
    m_hood=!m_hood;beginMotion("hood",QStringLiteral("前备厢正在开合"),m_hood?QStringLiteral("前备厢已打开"):QStringLiteral("前备厢已关闭"),hoodMotionDuration());emit changed();
}
void VehicleState::togglePort() {
    if(systemEnergyCommand())return;
    if(externalCommand("setPort",{{"value",!m_port}},QStringLiteral("充电口")))return;
    if(!parked() || m_plugged){emit toast(m_plugged?QStringLiteral("请先断开充电枪"):QStringLiteral("请挂 P 挡后操作充电口"),"warning");return;}
    m_port=!m_port;emit changed();
}
void VehicleState::toggleMirrors() {
    if(externalCommand("setMirrors",{{"value",!m_mirrors}},QStringLiteral("后视镜")))return;
    if(!parked()){emit toast(QStringLiteral("请挂 P 挡后折叠后视镜"),"warning");return;}
    m_mirrors=!m_mirrors;emit changed();
}
void VehicleState::cancelMotions() {
    m_motions.clear();m_motionCompletion.clear();m_windowBatch.clear();m_windowTimer.stop();
    m_windowMoving.fill(false);m_windowDragging.fill(false);
}
QVariantList VehicleState::notifications() const {
    QVariantList result;
    auto add=[&](const QString &part,const QString &title,const QString &detail,const QString &icon,const QString &severity){result.append(QVariantMap{{"part",part},{"title",title},{"detail",detail},{"icon",icon},{"severity",severity}});};
    for(int i=0;i<4;++i){
        const auto key=QString("door%1").arg(i),name=names[i]+QStringLiteral("车门");
        if(m_motions.contains(key))add(key,name+QStringLiteral("正在开合"),QStringLiteral("完成后更新状态"),"door","info");
        else if(m_doors[i])add(key,name+QStringLiteral("未关闭"),QStringLiteral("关闭后才能开始行驶演示"),"door","warning");
    }
    for(const auto &key:{QString("hood"),QString("hatch")}){
        const bool open=key=="hood"?m_hood:m_hatch;const auto name=key=="hood"?QStringLiteral("前备厢"):QStringLiteral("后备厢");
        if(m_motions.contains(key))add(key,name+QStringLiteral("正在开合"),QStringLiteral("完成后更新状态"),"trunk","info");
        else if(open)add(key,name+QStringLiteral("未关闭"),QStringLiteral("关闭后才能开始行驶演示"),"trunk","warning");
    }
    if(m_plugged)add("port",chargeStatus(),chargeControlAvailable()?QStringLiteral("充电目标 %1% · %2 kW").arg(m_chargeTarget).arg(chargePower(),0,'f',0):QStringLiteral("Windows 实际供电状态；功率与充电目标未提供"),"charge","info");
    else if(m_port)add("port",QStringLiteral("充电口已打开"),QStringLiteral("行驶前请关闭充电口"),"charge","warning");
    for(int i=0;i<4;++i){
        const auto key=QString("window%1").arg(i),name=names[i]+QStringLiteral("车窗");
        if(m_windowMoving[i]||m_windowDragging[i])add(key,name+QStringLiteral("正在调整"),QStringLiteral("当前 %1% → 目标 %2%").arg(qRound(m_windowActual[i])).arg(m_windowTargets[i]),"window","info");
        else if(m_windowActual[i]>0)add(key,name+QStringLiteral("已开启"),QStringLiteral("当前开度 %1%").arg(qRound(m_windowActual[i])),"window","info");
    }
    if(!m_locked)add("lock",QStringLiteral("车辆已解锁"),QStringLiteral("离车前请锁车"),"lock","info");
    if(m_mirrors)add("mirrors",QStringLiteral("后视镜已折叠"),QStringLiteral("挂 D 挡后自动展开"),"mirror","info");
    return result;
}
int VehicleState::warningCount() const {int count=0;for(const auto &entry:notifications())if(entry.toMap()["severity"]=="warning")++count;return count;}
QString VehicleState::statusTitle() const {
    if(busy())return QStringLiteral("车辆部件正在调整");const int warnings=warningCount();
    if(warnings)return QStringLiteral("%1 项部件需要关闭").arg(warnings);
    const int count=notifications().size();return count?QStringLiteral("%1 项当前状态提醒").arg(count):QStringLiteral("车辆状态正常");
}
