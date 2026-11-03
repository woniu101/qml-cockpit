#include "AudioDevices.h"
#include <QDesktopServices>
#include <QSettings>
#include <QUrl>

AudioDevices::AudioDevices(QAudioOutput *output,QObject *parent,bool live):QObject(parent),m_output(output){
    if(!qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS"))m_selected=QSettings().value("audio/outputId").toString();
    connect(&m_devices,&QMediaDevices::audioOutputsChanged,this,[this]{if(m_live)refresh();});
    setSamplingEnabled(live);
}
void AudioDevices::setSamplingEnabled(bool live){m_live=live;if(live)refresh();else emit changed();}
void AudioDevices::refresh(){if(m_live)applyDevices(QMediaDevices::audioOutputs(),QMediaDevices::defaultAudioOutput());}
void AudioDevices::applyDevices(const QList<QAudioDevice> &devices,const QAudioDevice &systemDefault){
    const bool hadOutput=available();m_outputs=devices;m_default=systemDefault;
    QStringList ids;for(const auto &device:devices)ids<<QString::fromLatin1(device.id().toHex());
    const auto route=resolveAudioRoute(ids,QString::fromLatin1(systemDefault.id().toHex()),m_selected);
    m_active={};for(const auto &device:devices)if(QString::fromLatin1(device.id().toHex())==route.id){m_active=device;break;}
    if(route.fellBack){m_selected.clear();saveSelection();if(available())emit notice(QStringLiteral("所选输出已不可用，已跟随系统默认设备"));}
    if(available())m_output->setDevice(m_active);
    emit changed();if(hadOutput&&!available())emit outputUnavailable();
}
QVariantList AudioDevices::outputs() const {
    QVariantList rows;
    for(const auto &device:m_outputs){const auto id=QString::fromLatin1(device.id().toHex());rows<<QVariantMap{{"id",id},{"name",device.description()},{"isDefault",device.id()==m_default.id()},{"selected",!followDefault()&&id==m_selected}};}
    return rows;
}
QString AudioDevices::statusText() const {
    if(!available())return QStringLiteral("未检测到音频输出，请连接扬声器或耳机");
    return (followDefault()?QStringLiteral("跟随系统默认 · "):QStringLiteral("本地音乐输出 · "))+activeName();
}
void AudioDevices::saveSelection(){if(qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS"))return;QSettings settings;settings.setValue("audio/outputId",m_selected);}
bool AudioDevices::selectOutput(const QString &id){
    if(m_live)refresh();
    if(!id.isEmpty()){
        bool found=false;for(const auto &device:m_outputs)if(QString::fromLatin1(device.id().toHex())==id)found=true;
        if(!found){emit notice(QStringLiteral("该输出已不可用，请重新选择"));return false;}
    }
    m_selected=id;saveSelection();applyDevices(m_outputs,m_default);return true;
}
void AudioDevices::openSettings(){if(!QDesktopServices::openUrl(QUrl("ms-settings:sound")))emit notice(QStringLiteral("无法打开系统声音设置，请从 Windows 设置进入"));}
