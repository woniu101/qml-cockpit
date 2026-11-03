#pragma once
#include <QObject>
#include <QVariantMap>
#include <QTimer>
#include <QAbstractNativeEventFilter>
#include <QStringList>

struct PowerSample {
    bool success=false;
    int batteryFlag=255,percent=255,ac=255,saver=255;
    QString error;
};
struct PowerReading {
    bool readOk=false,presenceKnown=false,hasBattery=false,socKnown=false;
    bool acKnown=false,onAc=false,chargingKnown=false,charging=false,saverKnown=false,saver=false,valid=false;
    int percent=-1;QString error;
};
struct NetworkSample {
    QString transport="unknown",reachability="unknown",backend;
    QStringList interfaces;
    bool captive=false,metered=false,meteredKnown=false;
};

class SystemStatus:public QObject,public QAbstractNativeEventFilter {
    Q_OBJECT
    Q_PROPERTY(QVariantMap power READ power NOTIFY powerChanged)
    Q_PROPERTY(QVariantMap network READ network NOTIFY networkChanged)
    Q_PROPERTY(bool sampling READ sampling NOTIFY samplingChanged)
public:
    explicit SystemStatus(QObject *parent=nullptr,bool live=true);
    ~SystemStatus() override;
    static PowerReading normalize(const PowerSample&);
    static PowerSample readPower();
    QVariantMap power() const;
    QVariantMap network() const;
    const PowerReading &powerReading() const{return m_power;}
    bool sampling() const{return m_live;}
    void setSamplingEnabled(bool enabled);
    void applyPowerSample(const PowerSample&);
    void applyNetworkSample(const NetworkSample&);
    Q_INVOKABLE void refresh();
    bool nativeEventFilter(const QByteArray&,void*,qintptr*) override;
signals:
    void powerChanged();void networkChanged();void samplingChanged();
private:
    void refreshNetwork();
    PowerReading m_power;NetworkSample m_network;
    QString m_powerUpdated,m_networkUpdated;
    QTimer m_timer;bool m_live=false;
};
