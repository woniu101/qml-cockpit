#pragma once
#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QThread>
#include <QTimer>
#include <QVariantMap>

struct BluetoothSnapshot {
    QString state="pending",error,deviceError;
    QVariantList adapters,devices;
};

class BluetoothStatus:public QObject,public QAbstractNativeEventFilter {
    Q_OBJECT
    Q_PROPERTY(QVariantMap status READ status NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool sampling READ sampling NOTIFY changed)
public:
    explicit BluetoothStatus(QObject *parent=nullptr,bool live=true);
    ~BluetoothStatus() override;
    QVariantMap status() const;
    bool busy() const{return m_busy;}
    bool sampling() const{return m_live;}
    void setSamplingEnabled(bool);
    void applySnapshot(const BluetoothSnapshot&);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void openSettings();
    bool nativeEventFilter(const QByteArray&,void*,qintptr*) override;
signals:
    void changed();
    void notice(const QString&);
private:
    static BluetoothSnapshot readSnapshot();
    BluetoothSnapshot m_snapshot;
    QString m_updated;
    QTimer m_timer;
    QThread *m_worker=nullptr;
    bool m_busy=false,m_live=false;
};
