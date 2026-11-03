#include "VehicleState.h"
#include <QJsonArray>

bool VehicleState::externalMode() const{return m_sourceMode=="external";}
bool VehicleState::externalReady() const{return !externalMode()||m_link->ready();}
void VehicleState::setExternalMode(bool enabled){
    setSourceMode(enabled?"external":"internal");
}
void VehicleState::setSourceMode(const QString &mode){
    if(!QStringList{"internal","system","external"}.contains(mode)||mode==m_sourceMode)return;
    if(!parked()&&!(externalMode()&&!m_link->ready())){emit toast(QStringLiteral("请挂 P 挡后切换数据来源"),"warning");return;}
    if(m_demoActive){emit toast(QStringLiteral("请先退出自动演示再切换数据来源"),"warning");return;}
    cancelMotions();m_remoteDragging.fill(false);m_remoteTripValid=false;
    if(externalMode()){
        m_sourceMode="internal";m_link->setEnabled(false);
        auto restored=snapshot();
        for(const char *key:{"locked","doors","hatch","hood","port","mirrors","windows","gear","speed","tripKm","navigation","destination","progress","battery","plugged","charging","chargeTarget"})restored[key]=m_localBeforeExternal[key];
        restoreSnapshot(restored);
    }
    if(systemMode()){
        if(m_systemFallback)m_energyBeforeSystem=energySnapshot();
        m_sourceMode="internal";restoreEnergy(m_energyBeforeSystem);
    }
    if(mode=="external"){m_localBeforeExternal=snapshot();m_sourceMode=mode;m_link->setEnabled(true);}
    else if(mode=="system"){
        m_energyBeforeSystem=energySnapshot();m_sourceMode=mode;m_systemFallback=false;m_systemHadSoc=false;
        m_plugged=m_charging=m_port=false;applySystemPower();
    }else m_sourceMode=mode;
    emit sourceChanged();emit changed();emit windowProgressChanged();emit motionsChanged();
}
bool VehicleState::externalCommand(const QString &kind,const QJsonObject &args,const QString &label){
    if(!externalMode())return false;const auto id=m_link->request(kind,args,label);
    if(!id.isEmpty()&&(kind=="setWindow"||kind=="stopWindow")){
        const int i=args["index"].toInt(-1);if(i>=0&&i<4){m_windowMessages[i]=kind=="setWindow"?QStringLiteral("目标 %1% · 等待模拟器确认").arg(args["target"].toInt()):QStringLiteral("等待模拟器确认停止");emit windowProgressChanged();}
    }
    return true;
}
void VehicleState::applyExternal(const QJsonObject &s){
    if(!externalMode())return;
    const bool newSession=m_remoteSession!=m_link->sessionId();
    if(newSession){m_remoteSession=m_link->sessionId();m_remoteTripValid=false;}
    const double forward=s["forwardKm"].toDouble();const double travelled=m_remoteTripValid?std::max(0.0,forward-m_remoteForward):0;m_remoteForward=forward;m_remoteTripValid=true;
    m_battery=s["soc"].toDouble();m_gear=s["gear"].toString();m_speed=s["speed"].toInt();m_tripKm=s["tripKm"].toDouble();m_locked=s["locked"].toBool();
    m_hood=s["hood"].toBool();m_hatch=s["hatch"].toBool();m_port=s["port"].toBool();m_mirrors=s["mirrors"].toBool();m_plugged=s["plugged"].toBool();m_charging=s["charging"].toBool();m_chargeTarget=s["chargeTarget"].toInt();m_remoteChargePower=s["chargePower"].toDouble();
    m_motions.clear();
    for(int i=0;i<4;++i){
        if((newSession||std::abs(m_windowActual[i]-s["windows"].toArray()[i].toDouble())>0.001)&&!s["windowMoving"].toArray()[i].toBool())m_windowMessages[i]=QStringLiteral("模拟器已上报：当前开度 %1%").arg(qRound(s["windows"].toArray()[i].toDouble()));
        m_windowActual[i]=s["windows"].toArray()[i].toDouble();m_windowTargets[i]=s["targets"].toArray()[i].toInt();m_windowMoving[i]=s["windowMoving"].toArray()[i].toBool();m_doors[i]=s["doors"].toArray()[i].toBool();
        if(s["doorMoving"].toArray()[i].toBool())m_motions.insert(QString("door%1").arg(i),{0,QStringLiteral("车门开合中")});
    }
    if(s["hoodMoving"].toBool())m_motions.insert("hood",{0,QStringLiteral("前备厢开合中")});if(s["hatchMoving"].toBool())m_motions.insert("hatch",{0,QStringLiteral("后备厢开合中")});
    if(m_navigating&&travelled>0){m_progress=std::min(1.0,m_progress+travelled/m_route.km);if(m_progress>=1){m_navigating=false;emit toast(QStringLiteral("已到达目的地，导航结束"));}}
    emit changed();emit windowProgressChanged();emit motionsChanged();emit sourceChanged();
}
void VehicleState::externalResult(const QString &kind,const QJsonObject &args,const QString &status,const QString &reason){
    if(!externalMode())return;
    if(kind=="setWindow"||kind=="stopWindow"){
        const int i=args["index"].toInt(-1);if(i>=0&&i<4){m_windowMessages[i]=status=="completed"?QStringLiteral("已确认：当前开度 %1%").arg(qRound(m_windowActual[i])):status=="accepted"?QStringLiteral("模拟器执行中"):reason;emit windowProgressChanged();}
    }
    if(status=="rejected"||status=="failed"||status=="timeout")emit toast(reason,"warning");
    else if(status=="completed"&&kind!="setWindow"&&kind!="stopWindow"&&kind!="setSpeed"&&kind!="advance")emit toast(QStringLiteral("模拟器已确认操作完成"));
}
