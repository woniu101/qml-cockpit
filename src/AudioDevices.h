#pragma once
#include <QObject>
#include <QAudioDevice>
#include <QAudioOutput>
#include <QMediaDevices>
#include <QVariantList>
#include "AudioRoutePolicy.h"

class AudioDevices:public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList outputs READ outputs NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(bool followDefault READ followDefault NOTIFY changed)
    Q_PROPERTY(QString activeName READ activeName NOTIFY changed)
    Q_PROPERTY(QString defaultName READ defaultName NOTIFY changed)
    Q_PROPERTY(QString statusText READ statusText NOTIFY changed)
    Q_PROPERTY(bool sampling READ sampling NOTIFY changed)
public:
    explicit AudioDevices(QAudioOutput *output,QObject *parent=nullptr,bool live=true);
    QVariantList outputs() const;
    bool available() const{return !m_active.isNull();}
    bool followDefault() const{return m_selected.isEmpty();}
    QString activeName() const{return available()?m_active.description():QStringLiteral("无可用输出设备");}
    QString defaultName() const{return m_default.isNull()?QStringLiteral("由系统选择"):m_default.description();}
    QString statusText() const;
    bool sampling() const{return m_live;}
    bool routeMatches() const{return available()&&m_output->device().id()==m_active.id();}
    void setSamplingEnabled(bool);
    void applyDevices(const QList<QAudioDevice>&,const QAudioDevice &systemDefault);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool selectOutput(const QString &id);
    Q_INVOKABLE void openSettings();
signals:
    void changed();
    void notice(const QString&);
    void outputUnavailable();
private:
    void saveSelection();
    QAudioOutput *m_output;
    QMediaDevices m_devices;
    QList<QAudioDevice> m_outputs;
    QAudioDevice m_default,m_active;
    QString m_selected;
    bool m_live=false;
};
