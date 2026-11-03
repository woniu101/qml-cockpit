#pragma once
#include <QObject>
#include <QThread>
#include <QVariantList>
#include <functional>
#include <memory>

// Backend instances and their native resources belong exclusively to the worker.
struct ReceiverScan { bool supported=true; QVariantList devices; QString error; };
struct ReceiverOpen { bool connected=false; QString error; };
class ReceiverBackend {
public:
    using Cancelled=std::function<bool()>;
    virtual ~ReceiverBackend()=default;
    virtual ReceiverScan scan(const Cancelled&)=0;
    virtual ReceiverOpen open(const QString &token,const Cancelled&)=0;
    virtual bool connected()=0;
    virtual void close() noexcept=0;
};

class BluetoothAudio:public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList devices READ devices NOTIFY changed)
    Q_PROPERTY(QString phase READ phase NOTIFY changed)
    Q_PROPERTY(QString text READ text NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QString scanError READ scanError NOTIFY changed)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY changed)
    Q_PROPERTY(bool supported READ supported NOTIFY changed)
    Q_PROPERTY(bool scanning READ scanning NOTIFY changed)
    Q_PROPERTY(bool active READ active NOTIFY changed)
    Q_PROPERTY(bool outputAvailable READ outputAvailable NOTIFY changed)
public:
    using Factory=std::function<std::unique_ptr<ReceiverBackend>()>;
    explicit BluetoothAudio(QObject *parent=nullptr,Factory factory={},bool enabled=true);
    ~BluetoothAudio() override;
    QVariantList devices() const{return m_devices;}
    QString phase() const{return m_phase;}
    QString text() const;
    QString error() const{return m_error;}
    QString scanError() const{return m_scanError;}
    QString deviceName() const{return m_name;}
    bool supported() const{return m_supported;}
    bool scanning() const{return m_scanning;}
    bool active() const{return m_phase=="connecting"||m_phase=="connected"||m_phase=="disconnecting";}
    bool outputAvailable() const{return m_outputAvailable;}
    void setOutputAvailable(bool available);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool connectDevice(const QString &token);
    Q_INVOKABLE void disconnectDevice();
signals:
    void changed();
    void notice(const QString&);
private:
    struct Shared;
    void run(Factory);
    void command(int type,const QString &token={});
    void acceptPhase(quint64 revision,const QString &phase,const QString &error={});
    std::shared_ptr<Shared> m_shared;
    QThread *m_worker=nullptr;
    QVariantList m_devices;
    QString m_phase="idle",m_error,m_scanError,m_name;
    bool m_supported=false,m_scanning=false,m_outputAvailable=true,m_enabled=true;
};
