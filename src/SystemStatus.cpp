#include "SystemStatus.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QNetworkInformation>
#include <QNetworkInterface>
#include <QHostAddress>
#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#endif

SystemStatus::SystemStatus(QObject *parent,bool live):QObject(parent){
    m_timer.setInterval(2000);connect(&m_timer,&QTimer::timeout,this,&SystemStatus::refresh);
    setSamplingEnabled(live);
}
SystemStatus::~SystemStatus(){if(qApp)qApp->removeNativeEventFilter(this);}
PowerReading SystemStatus::normalize(const PowerSample &raw){
    PowerReading p;p.readOk=raw.success;p.error=raw.error;if(!raw.success)return p;
    // 255 means unknown, and must be handled before testing the no-battery bit.
    p.presenceKnown=raw.batteryFlag>=0&&raw.batteryFlag<255;
    p.hasBattery=p.presenceKnown&&!(raw.batteryFlag&128);
    p.acKnown=raw.ac==0||raw.ac==1;p.onAc=raw.ac==1;
    p.socKnown=p.hasBattery&&raw.percent>=0&&raw.percent<=100;if(p.socKnown)p.percent=raw.percent;
    p.chargingKnown=p.hasBattery&&p.presenceKnown;p.charging=p.chargingKnown&&(raw.batteryFlag&8);
    p.saverKnown=raw.saver==0||raw.saver==1;p.saver=raw.saver==1;
    p.valid=p.hasBattery&&p.socKnown&&p.acKnown&&p.chargingKnown&&!(p.charging&&!p.onAc);
    return p;
}
PowerSample SystemStatus::readPower(){
    PowerSample raw;
#ifdef Q_OS_WIN
    SYSTEM_POWER_STATUS status{};raw.success=GetSystemPowerStatus(&status);
    if(raw.success){raw.batteryFlag=status.BatteryFlag;raw.percent=status.BatteryLifePercent;raw.ac=status.ACLineStatus;raw.saver=status.SystemStatusFlag;}
    else raw.error=QStringLiteral("系统电源读取失败（%1）").arg(GetLastError());
#else
    raw.error=QStringLiteral("当前平台不支持 Windows 电源映射");
#endif
    return raw;
}
void SystemStatus::setSamplingEnabled(bool enabled){
    if(enabled==m_live)return;m_live=enabled;
    if(enabled){
        if(qApp)qApp->installNativeEventFilter(this);
        QNetworkInformation::loadDefaultBackend();
        if(auto *info=QNetworkInformation::instance()){
            disconnect(info,nullptr,this,nullptr);
            connect(info,&QNetworkInformation::reachabilityChanged,this,[this]{if(m_live)refreshNetwork();});
            connect(info,&QNetworkInformation::transportMediumChanged,this,[this]{if(m_live)refreshNetwork();});
            connect(info,&QNetworkInformation::isBehindCaptivePortalChanged,this,[this]{if(m_live)refreshNetwork();});
            connect(info,&QNetworkInformation::isMeteredChanged,this,[this]{if(m_live)refreshNetwork();});
        }
        refresh();m_timer.start();
    }else{m_timer.stop();if(qApp)qApp->removeNativeEventFilter(this);}
    emit samplingChanged();
}
void SystemStatus::applyPowerSample(const PowerSample &sample){m_power=normalize(sample);m_powerUpdated=QTime::currentTime().toString("HH:mm:ss");emit powerChanged();}
void SystemStatus::applyNetworkSample(const NetworkSample &sample){m_network=sample;m_networkUpdated=QTime::currentTime().toString("HH:mm:ss");emit networkChanged();}
void SystemStatus::refresh(){if(!m_live)return;applyPowerSample(readPower());refreshNetwork();}
bool SystemStatus::nativeEventFilter(const QByteArray&,void *message,qintptr*){
#ifdef Q_OS_WIN
    const auto *event=static_cast<MSG*>(message);
    if(m_live&&event->message==WM_POWERBROADCAST){
        if(event->wParam==PBT_APMSUSPEND){m_power.valid=false;emit powerChanged();}
        else QTimer::singleShot(0,this,&SystemStatus::refresh);
    }
#else
    Q_UNUSED(message);
#endif
    return false;
}
QVariantMap SystemStatus::power() const {
    const auto &p=m_power;QString status;
    if(!p.readOk)status=p.error.isEmpty()?QStringLiteral("电脑电源状态未知"):p.error;
    else if(p.presenceKnown&&!p.hasBattery)status=QStringLiteral("未检测到系统电池");
    else if(!p.valid)status=QStringLiteral("电源信息不完整，等待系统更新");
    else if(p.charging)status=QStringLiteral("系统正在充电");
    else if(p.onAc)status=p.percent==100?QStringLiteral("外接电源 · 电池已满"):QStringLiteral("外接电源 · 未充电");
    else status=QStringLiteral("电池供电");
    return {{"readOk",p.readOk},{"presenceKnown",p.presenceKnown},{"hasBattery",p.hasBattery},{"socKnown",p.socKnown},{"percent",p.percent},
        {"acKnown",p.acKnown},{"onAc",p.onAc},{"chargingKnown",p.chargingKnown},{"charging",p.charging},{"saverKnown",p.saverKnown},{"saver",p.saver},
        {"valid",p.valid},{"statusText",status},{"updated",m_powerUpdated}};
}
void SystemStatus::refreshNetwork(){
    NetworkSample n;auto *info=QNetworkInformation::instance();
    if(info){
        n.backend=info->backendName();
        if(info->supports(QNetworkInformation::Feature::Reachability)){
            switch(info->reachability()){
            case QNetworkInformation::Reachability::Disconnected:n.reachability="offline";break;
            case QNetworkInformation::Reachability::Local:n.reachability="local";break;
            case QNetworkInformation::Reachability::Site:n.reachability="site";break;
            case QNetworkInformation::Reachability::Online:n.reachability="online";break;
            default:break;
            }
        }
        if(info->supports(QNetworkInformation::Feature::TransportMedium)){
            switch(info->transportMedium()){
            case QNetworkInformation::TransportMedium::Ethernet:n.transport="ethernet";break;
            case QNetworkInformation::TransportMedium::WiFi:n.transport="wifi";break;
            case QNetworkInformation::TransportMedium::Cellular:n.transport="cellular";break;
            case QNetworkInformation::TransportMedium::Bluetooth:n.transport="bluetooth";break;
            default:break;
            }
        }
        if(info->supports(QNetworkInformation::Feature::CaptivePortal))n.captive=info->isBehindCaptivePortal();
        n.meteredKnown=info->supports(QNetworkInformation::Feature::Metered);if(n.meteredKnown)n.metered=info->isMetered();
    }
    QStringList physicalTypes;bool otherActive=false;
    for(const auto &nic:QNetworkInterface::allInterfaces()){
        const auto flags=nic.flags();if(!flags.testFlag(QNetworkInterface::IsUp)||!flags.testFlag(QNetworkInterface::IsRunning)||flags.testFlag(QNetworkInterface::IsLoopBack))continue;
        bool hasAddress=false;for(const auto &address:nic.addressEntries())if(!address.ip().isNull()&&!address.ip().isLoopback()&&!address.ip().isLinkLocal())hasAddress=true;
        if(!hasAddress)continue;
        bool hardware=nic.type()==QNetworkInterface::Ethernet||nic.type()==QNetworkInterface::Wifi;
#ifdef Q_OS_WIN
        MIB_IF_ROW2 row{};row.InterfaceIndex=ULONG(nic.index());if(GetIfEntry2(&row)==NO_ERROR)hardware=row.InterfaceAndOperStatusFlags.HardwareInterface;
#endif
        const QString type=nic.type()==QNetworkInterface::Wifi?"wifi":nic.type()==QNetworkInterface::Ethernet?"ethernet":"unknown";
        const auto label=hardware&&type=="ethernet"?QStringLiteral("有线"):hardware&&type=="wifi"?QStringLiteral("Wi-Fi"):QStringLiteral("虚拟或其他网络");
        n.interfaces<<nic.humanReadableName()+QStringLiteral(" · ")+label;
        if(hardware&&type!="unknown")physicalTypes<<type;else otherActive=true;
    }
    physicalTypes.removeDuplicates();
    // Without an OS-selected medium, multiple active paths must remain ambiguous.
    if(n.transport=="unknown"){
        if(!otherActive&&physicalTypes.size()==1)n.transport=physicalTypes.first();
        else if(n.interfaces.size()>1)n.transport="mixed";
        else if(n.interfaces.isEmpty()&&n.reachability!="online")n.transport="none";
    }
    if(n.reachability=="offline")n.transport="none";
    applyNetworkSample(n);
}
QVariantMap SystemStatus::network() const {
    const auto &n=m_network;
    const QString type=n.transport=="ethernet"?QStringLiteral("有线"):n.transport=="wifi"?QString("Wi-Fi"):n.transport=="cellular"?QStringLiteral("蜂窝"):n.transport=="bluetooth"?QStringLiteral("蓝牙网络"):n.transport=="mixed"?QStringLiteral("多网"):n.transport=="none"?QStringLiteral("离线"):QStringLiteral("未知");
    const QString state=n.captive?QStringLiteral("需要网络认证"):n.reachability=="online"?QStringLiteral("系统报告可访问互联网"):n.reachability=="offline"?QStringLiteral("未连接网络"):n.reachability=="local"?QStringLiteral("仅本地网络"):n.reachability=="site"?QStringLiteral("本地或内部网络"):QStringLiteral("联网状态未知");
    const QString icon=n.transport=="ethernet"?"ethernet":n.transport=="wifi"?"wifi":n.transport=="cellular"?"signal":n.transport=="none"?"network-off":"network";
    return {{"transport",n.transport},{"reachability",n.reachability},{"label",type==QStringLiteral("蓝牙网络")?QStringLiteral("蓝牙"):type},{"typeText",type},
        {"stateText",state},{"summary",type+QStringLiteral(" · ")+state},{"icon",icon},{"interfaces",n.interfaces},{"meteredKnown",n.meteredKnown},{"metered",n.metered},{"captive",n.captive},{"updated",m_networkUpdated},{"backend",n.backend}};
}
