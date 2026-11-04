#pragma once
#include <QObject>
#include <QLocalSocket>
#include <QTimer>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QHash>

class VehicleLink:public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled NOTIFY statusChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY statusChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QString serverName READ serverName WRITE setServerName NOTIFY statusChanged)
    Q_PROPERTY(QString lastCommand READ lastCommand NOTIFY activityChanged)
    Q_PROPERTY(QStringList events READ events NOTIFY activityChanged)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY activityChanged)
public:
    explicit VehicleLink(QObject *parent=nullptr);
    ~VehicleLink() override;
    bool enabled() const{return m_enabled;}
    bool ready() const{return m_enabled&&m_ready;}
    QString statusText() const{return m_status;}
    QString serverName() const{return m_serverName;}
    QString sessionId() const{return m_session;}
    QString lastCommand() const{return m_lastCommand;}
    QStringList events() const{return m_events;}
    int pendingCount() const{return m_pending.size();}
    void setEnabled(bool);
    void setServerName(const QString&);
    Q_INVOKABLE void reconnect();
    QString request(const QString&,const QJsonObject&,const QString&);
signals:
    void statusChanged();void activityChanged();
    void snapshotReceived(const QJsonObject &state);
    void commandResult(const QString &kind,const QJsonObject &args,const QString &status,const QString &reason,const QString &id);
private:
    struct Pending{QString kind,label;QJsonObject args;qint64 deadline;bool accepted=false;};
    void read();void message(const QJsonObject&);void tick();void invalidate(const QString&);void failPending(const QString&);void log(const QString&);
    QLocalSocket m_socket;QTimer m_timer;QElapsedTimer m_clock;
    QByteArray m_buffer;QString m_serverName="qml-cockpit.vehicle.v1",m_session,m_status=QStringLiteral("内置演示"),m_lastCommand;
    QStringList m_events;QHash<QString,Pending> m_pending;
    bool m_enabled=false,m_ready=false;quint64 m_sequence=0;qint64 m_lastSnapshot=0,m_nextAttempt=0,m_connectedAt=0;
};
