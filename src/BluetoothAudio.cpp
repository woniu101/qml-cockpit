#include "BluetoothAudio.h"
#include <QTimer>
#include <QScopeGuard>
#include <QHash>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <thread>
#include <stdexcept>
#ifdef Q_OS_WIN
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Media.Audio.h>
#endif

namespace {
enum Command { None, Scan, Connect, Disconnect };
using namespace std::chrono_literals;
#ifdef Q_OS_WIN
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Media::Audio;

QString nativeError(const winrt::hresult_error &error){
    return QStringLiteral("Windows 错误 0x%1").arg(quint32(error.code().value),8,16,QChar('0'));
}
template<class Async> void await(const Async &operation,const ReceiverBackend::Cancelled &cancel,int seconds){
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(seconds);
    for(;;){
        if(cancel()){operation.Cancel();throw std::runtime_error("cancelled");}
        // C++/WinRT wait_for installs Completed; invoking it repeatedly is illegal
        // for these one-handler WinRT operations (E_ILLEGAL_DELEGATE_ASSIGNMENT).
        const auto status=operation.Status();
        if(status==AsyncStatus::Completed)return;
        if(status==AsyncStatus::Error||status==AsyncStatus::Canceled){operation.GetResults();throw std::runtime_error("operation cancelled");}
        if(std::chrono::steady_clock::now()>=deadline){operation.Cancel();throw std::runtime_error("timeout");}
        std::this_thread::sleep_for(100ms);
    }
}
class WindowsReceiver final:public ReceiverBackend {
    AudioPlaybackConnection m_connection{nullptr};
    QHash<QString,winrt::hstring> m_ids;
    QHash<QString,QString> m_tokens;
    int m_nextToken=0;
public:
    WindowsReceiver(){winrt::init_apartment(winrt::apartment_type::multi_threaded);}
    ~WindowsReceiver() override{close();winrt::uninit_apartment();}
    ReceiverScan scan(const Cancelled &cancel) override{
        ReceiverScan result;
        if(!winrt::Windows::Foundation::Metadata::ApiInformation::IsTypePresent(L"Windows.Media.Audio.AudioPlaybackConnection")){
            result.supported=false;result.error=QStringLiteral("系统不支持手机音乐接收，需要 Windows 10 2004 或更新版本");return result;
        }
        try{
            // This selector returns audio-reception endpoints, not generic Bluetooth devices.
            auto query=winrt::Windows::Devices::Enumeration::DeviceInformation::FindAllAsync(AudioPlaybackConnection::GetDeviceSelector());
            await(query,cancel,5);QHash<QString,winrt::hstring> ids;
            for(const auto &device:query.GetResults()){
                const QString id=QString::fromWCharArray(device.Id().c_str());
                if(!m_tokens.contains(id))m_tokens.insert(id,QString::number(++m_nextToken));
                const auto token=m_tokens.value(id);ids.insert(token,device.Id());
                const auto name=QString::fromWCharArray(device.Name().c_str()).left(160);
                result.devices<<QVariantMap{{"token",token},{"name",name.isEmpty()?QStringLiteral("未命名音频设备"):name}};
            }
            m_ids=ids;
        }catch(const winrt::hresult_error &error){result.error=QStringLiteral("无法读取手机音频设备，")+nativeError(error);}
        catch(const std::exception &){result.error=QStringLiteral("手机音频设备查询超时，请刷新重试");}
        return result;
    }
    ReceiverOpen open(const QString &token,const Cancelled &cancel) override{
        close();ReceiverOpen result;
        if(!m_ids.contains(token)){result.error=QStringLiteral("设备已移除，请刷新后重新选择");return result;}
        try{
            m_connection=AudioPlaybackConnection::TryCreateFromId(m_ids.value(token));
            if(!m_connection){result.error=QStringLiteral("该设备不支持音乐接收，请确认手机已配对");return result;}
            auto start=m_connection.StartAsync();await(start,cancel,10);start.GetResults();
            auto open=m_connection.OpenAsync();await(open,cancel,25);const auto opened=open.GetResults();
            const auto status=opened.Status();
            if(status==AudioPlaybackConnectionOpenResultStatus::Success&&connected()&&!cancel())result.connected=true;
            else if(status==AudioPlaybackConnectionOpenResultStatus::RequestTimedOut)result.error=QStringLiteral("手机连接超时，请确认手机蓝牙已开启后重试");
            else if(status==AudioPlaybackConnectionOpenResultStatus::DeniedBySystem)result.error=QStringLiteral("Windows 拒绝了音乐接收，请检查蓝牙设置及是否有其它接收程序占用");
            else result.error=QStringLiteral("手机音频连接未建立，请重试（0x%1）").arg(quint32(opened.ExtendedError().value),8,16,QChar('0'));
        }catch(const winrt::hresult_error &error){result.error=QStringLiteral("手机音乐连接失败，")+nativeError(error);}
        catch(const std::exception &){result.error=QStringLiteral("手机音乐连接超时，请重试");}
        if(!result.connected)close();return result;
    }
    bool connected() override{return m_connection&&m_connection.State()==AudioPlaybackConnectionState::Opened;}
    void close() noexcept override{if(m_connection){try{m_connection.Close();}catch(...){}m_connection=nullptr;}}
};
#else
class WindowsReceiver final:public ReceiverBackend {
public:
    ReceiverScan scan(const Cancelled&) override{return {false,{},QStringLiteral("当前平台不支持 Windows 蓝牙音乐接收")};}
    ReceiverOpen open(const QString&,const Cancelled&) override{return {};}
    bool connected() override{return false;}
    void close() noexcept override{}
};
#endif
}

struct BluetoothAudio::Shared {
    std::atomic_bool stop{false};
    std::atomic<quint64> revision{0};
    std::mutex mutex;
    std::condition_variable wake;
    int command=None;
    QString token;
};
BluetoothAudio::BluetoothAudio(QObject *parent,Factory factory,bool enabled):QObject(parent),m_shared(std::make_shared<Shared>()),m_enabled(enabled){
    if(!enabled){m_phase="unsupported";return;}
    m_worker=QThread::create([this,factory]{run(factory);});m_worker->setParent(this);m_worker->start();
    auto *timer=new QTimer(this);timer->setInterval(10000);connect(timer,&QTimer::timeout,this,&BluetoothAudio::refresh);timer->start();
    QTimer::singleShot(0,this,&BluetoothAudio::refresh);
}
BluetoothAudio::~BluetoothAudio(){m_shared->stop=true;++m_shared->revision;m_shared->wake.notify_one();if(m_worker)m_worker->wait();}
QString BluetoothAudio::text() const{
    return m_phase=="connected"?QStringLiteral("手机音频已接通"):m_phase=="connecting"?QStringLiteral("正在连接手机音乐…"):m_phase=="disconnecting"?QStringLiteral("正在断开手机音乐…"):m_phase=="unsupported"?QStringLiteral("手机音乐接收不可用"):m_phase=="error"?QStringLiteral("手机音乐连接未建立"):QStringLiteral("手机音乐未连接");
}
void BluetoothAudio::command(int type,const QString &token){
    {std::lock_guard lock(m_shared->mutex);++m_shared->revision;m_shared->command=type;m_shared->token=token;}
    m_shared->wake.notify_one();
}
void BluetoothAudio::refresh(){
    if(!m_enabled||m_scanning||m_phase=="connecting"||m_phase=="disconnecting")return;
    m_scanning=true;command(Scan);emit changed();
}
bool BluetoothAudio::connectDevice(const QString &token){
    if(!m_enabled||!m_supported||m_scanning||!m_scanError.isEmpty()||active()||!m_outputAvailable)return false;
    QString name;for(const auto &row:m_devices)if(row.toMap().value("token")==token)name=row.toMap().value("name").toString();
    if(name.isEmpty())return false;
    m_name=name;m_error.clear();m_phase="connecting";command(Connect,token);emit changed();return true;
}
void BluetoothAudio::disconnectDevice(){
    if(!active()||m_phase=="disconnecting")return;
    m_phase="disconnecting";m_scanning=false;command(Disconnect);emit changed();
}
void BluetoothAudio::setOutputAvailable(bool available){
    if(m_outputAvailable==available)return;m_outputAvailable=available;
    if(!available&&active()){disconnectDevice();emit notice(QStringLiteral("音频输出已移除，手机音乐接收已停止"));}
    emit changed();
}
void BluetoothAudio::acceptPhase(quint64 revision,const QString &phase,const QString &error){
    if(m_shared->revision!=revision)return;
    m_phase=phase;m_error=error;emit changed();
}
void BluetoothAudio::run(Factory factory){
    auto shared=m_shared;
    auto deliver=[this,shared](quint64 revision,auto function){
        if(shared->stop)return;
        QMetaObject::invokeMethod(this,[shared,revision,function]{if(!shared->stop&&shared->revision==revision)function();},Qt::QueuedConnection);
    };
    try{
        auto backend=factory?factory():std::make_unique<WindowsReceiver>();
        auto cleanup=qScopeGuard([&]{backend->close();});bool open=false;QString activeToken;
        for(;;){
            int task;QString token;quint64 revision;
            {std::unique_lock lock(shared->mutex);shared->wake.wait_for(lock,500ms,[&]{return shared->stop||shared->command!=None;});if(shared->stop)break;task=shared->command;shared->command=None;token=shared->token;revision=shared->revision;}
            const auto cancelled=[shared,revision]{return shared->stop||shared->revision!=revision;};
            if(task==Disconnect){backend->close();open=false;deliver(revision,[this,revision]{acceptPhase(revision,"idle");});}
            else if(task==Scan){
                const auto result=backend->scan(cancelled);
                if(!cancelled())deliver(revision,[this,result]{m_scanning=false;m_supported=result.supported;m_scanError=result.error;if(result.error.isEmpty())m_devices=result.devices;if(!result.supported)m_phase="unsupported";else if(m_phase=="unsupported")m_phase="idle";emit changed();});
                bool found=false;for(const auto &row:result.devices)found|=row.toMap().value("token")==activeToken;
                if(open&&result.error.isEmpty()&&!found){backend->close();open=false;deliver(revision,[this,revision]{acceptPhase(revision,"error",QStringLiteral("手机音频设备已移除，请重新配对或连接适配器后刷新"));});}
            }else if(task==Connect){
                const auto result=backend->open(token,cancelled);open=result.connected;activeToken=token;
                if(cancelled()){backend->close();open=false;continue;}
                deliver(revision,[this,revision,result]{acceptPhase(revision,result.connected?"connected":"error",result.error);});
            }
            if(open&&!cancelled()){
                bool connected=false;try{connected=backend->connected();}catch(...){}
                if(!connected){backend->close();open=false;deliver(revision,[this,revision]{acceptPhase(revision,"error",QStringLiteral("手机音频已断开，请确认手机和适配器状态后重新连接"));});}
            }
        }
    }catch(...){
        const auto revision=shared->revision.load();deliver(revision,[this,revision]{m_scanning=false;m_supported=false;m_enabled=false;acceptPhase(revision,"error",QStringLiteral("手机音乐服务不可用，请重新启动应用后重试"));});
    }
}
