#include "BluetoothStatus.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QScopeGuard>
#include <memory>
#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Radios.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <chrono>
#endif

BluetoothStatus::BluetoothStatus(QObject *parent,bool live):QObject(parent){
    m_timer.setInterval(10000);connect(&m_timer,&QTimer::timeout,this,&BluetoothStatus::refresh);setSamplingEnabled(live);
}
BluetoothStatus::~BluetoothStatus(){
    m_live=false;if(qApp)qApp->removeNativeEventFilter(this);
    if(m_worker){m_worker->requestInterruption();m_worker->wait();}
}
void BluetoothStatus::setSamplingEnabled(bool live){
    m_live=live;
    if(live){if(qApp)qApp->installNativeEventFilter(this);m_timer.start();QTimer::singleShot(0,this,&BluetoothStatus::refresh);}
    else{m_timer.stop();if(qApp)qApp->removeNativeEventFilter(this);if(m_worker)m_worker->requestInterruption();}
    emit changed();
}
void BluetoothStatus::applySnapshot(const BluetoothSnapshot &snapshot){m_snapshot=snapshot;m_updated=QTime::currentTime().toString("HH:mm:ss");emit changed();}
void BluetoothStatus::refresh(){
    if(!m_live||m_busy)return;m_busy=true;emit changed();
    auto result=std::make_shared<BluetoothSnapshot>();
    auto *thread=QThread::create([result]{*result=readSnapshot();});m_worker=thread;thread->setParent(this);
    connect(thread,&QThread::finished,this,[this,result,thread]{m_worker=nullptr;m_busy=false;if(m_live)applySnapshot(*result);else emit changed();thread->deleteLater();});
    thread->start();
}
QVariantMap BluetoothStatus::status() const {
    const auto &s=m_snapshot;int connected=0;for(const auto &row:s.devices){const auto value=row.toMap();if(value.value("connectionKnown").toBool()&&value.value("connected").toBool())++connected;}
    const QString text=s.state=="on"?(connected?QStringLiteral("蓝牙已开启 · 有设备连接"):QStringLiteral("蓝牙已开启")):s.state=="off"?QStringLiteral("蓝牙已关闭"):s.state=="disabled"?QStringLiteral("蓝牙被系统禁用"):s.state=="absent"?QStringLiteral("未检测到蓝牙适配器"):s.state=="denied"?QStringLiteral("蓝牙状态访问受限"):s.state=="unsupported"?QStringLiteral("当前平台不支持蓝牙状态读取"):s.state=="pending"?QStringLiteral("正在读取蓝牙状态"):s.state=="unknown"?QStringLiteral("蓝牙状态待确认"):QStringLiteral("蓝牙状态读取失败");
    return {{"state",s.state},{"text",text},{"connectedCount",connected},{"adapters",s.adapters},{"devices",s.devices},{"error",s.error},{"deviceError",s.deviceError},{"updated",m_updated}};
}
bool BluetoothStatus::nativeEventFilter(const QByteArray&,void *message,qintptr*){
#ifdef Q_OS_WIN
    const auto *event=static_cast<MSG*>(message);
    if(m_live&&(event->message==WM_DEVICECHANGE||event->message==WM_POWERBROADCAST))QTimer::singleShot(0,this,&BluetoothStatus::refresh);
#else
    Q_UNUSED(message);
#endif
    return false;
}
void BluetoothStatus::openSettings(){if(!QDesktopServices::openUrl(QUrl("ms-settings:bluetooth")))emit notice(QStringLiteral("无法打开系统蓝牙设置，请从 Windows 设置进入"));}

BluetoothSnapshot BluetoothStatus::readSnapshot(){
    BluetoothSnapshot sample;
#ifdef Q_OS_WIN
    using namespace winrt::Windows::Devices;
    using namespace winrt::Windows::Foundation;
    using namespace std::chrono_literals;
    try{
        winrt::init_apartment(winrt::apartment_type::multi_threaded);auto apartment=qScopeGuard([]{winrt::uninit_apartment();});
        auto radios=Radios::Radio::GetRadiosAsync();
        if(radios.wait_for(3s)!=AsyncStatus::Completed){radios.Cancel();sample.state="error";sample.error=QStringLiteral("系统蓝牙查询超时，请稍后刷新");return sample;}
        bool anyOn=false,anyOff=false,anyDisabled=false;
        for(const auto &radio:radios.GetResults())if(radio.Kind()==Radios::RadioKind::Bluetooth){
            const auto state=radio.State();const QString value=state==Radios::RadioState::On?"on":state==Radios::RadioState::Off?"off":state==Radios::RadioState::Disabled?"disabled":"unknown";
            anyOn|=value=="on";anyOff|=value=="off";anyDisabled|=value=="disabled";
            sample.adapters<<QVariantMap{{"name",QString::fromWCharArray(radio.Name().c_str()).left(160)},{"state",value}};
        }
        if(sample.adapters.isEmpty()){
            // An empty WinRT collection alone does not prove that no adapter exists.
            const auto list=SetupDiGetClassDevsW(&GUID_DEVCLASS_BLUETOOTH,nullptr,nullptr,DIGCF_PRESENT);
            if(list==INVALID_HANDLE_VALUE){sample.state="unknown";return sample;}
            auto cleanup=qScopeGuard([list]{SetupDiDestroyDeviceInfoList(list);});SP_DEVINFO_DATA info{};info.cbSize=sizeof(info);int present=0;bool allDisabled=true;
            for(DWORD i=0;SetupDiEnumDeviceInfo(list,i,&info);++i){++present;ULONG flags=0,problem=0;if(CM_Get_DevNode_Status(&flags,&problem,info.DevInst,0)!=CR_SUCCESS||problem!=CM_PROB_DISABLED)allDisabled=false;}
            const auto error=GetLastError();sample.state=error!=ERROR_NO_MORE_ITEMS?"unknown":present==0?"absent":allDisabled?"disabled":"unknown";return sample;
        }
        sample.state=anyOn?"on":anyOff?"off":anyDisabled?"disabled":"unknown";
        if(!anyOn||QThread::currentThread()->isInterruptionRequested())return sample;
        auto properties=winrt::single_threaded_vector<winrt::hstring>({L"System.Devices.Aep.IsConnected"});
        auto addDevices=[&](const winrt::hstring &selector,const QString &kind){
            auto query=Enumeration::DeviceInformation::FindAllAsync(selector,properties,Enumeration::DeviceInformationKind::AssociationEndpoint);
            if(query.wait_for(3s)!=AsyncStatus::Completed){query.Cancel();sample.deviceError=QStringLiteral("部分已配对设备查询超时，请刷新重试");return;}
            for(const auto &device:query.GetResults()){
                if(!device.Pairing().IsPaired())continue;
                const auto property=device.Properties().TryLookup(L"System.Devices.Aep.IsConnected");const auto connected=property?property.try_as<IPropertyValue>():nullptr;
                const bool known=connected&&connected.Type()==PropertyType::Boolean;
                const auto name=QString::fromWCharArray(device.Name().c_str()).left(160);
                sample.devices<<QVariantMap{{"name",name.isEmpty()?QStringLiteral("未命名设备"):name},{"kind",kind},{"connected",known&&connected.GetBoolean()},{"connectionKnown",known}};
            }
        };
        try{addDevices(Bluetooth::BluetoothDevice::GetDeviceSelectorFromPairingState(true),QStringLiteral("经典蓝牙"));if(!QThread::currentThread()->isInterruptionRequested())addDevices(Bluetooth::BluetoothLEDevice::GetDeviceSelectorFromPairingState(true),QStringLiteral("低功耗蓝牙"));}
        catch(const winrt::hresult_error&){sample.deviceError=QStringLiteral("部分已配对设备信息不可用，请在系统设置中查看");}
    }catch(const winrt::hresult_error &error){sample.state=error.code()==E_ACCESSDENIED?"denied":"error";sample.error=QStringLiteral("Windows 返回 0x%1").arg(quint32(error.code().value),8,16,QChar('0'));}
    catch(...){sample.state="error";sample.error=QStringLiteral("系统蓝牙查询未完成，请刷新重试");}
#else
    sample.state="unsupported";
#endif
    return sample;
}
