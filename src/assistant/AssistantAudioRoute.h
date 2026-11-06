#pragma once

#include <QAudioDevice>
#include <QMediaDevices>
#include <QStringList>

class AssistantAudioRoute final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList names READ names NOTIFY changed)
    Q_PROPERTY(int selectedIndex READ selectedIndex NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY changed)
public:
    explicit AssistantAudioRoute(QAudioDevice::Mode mode);
    QStringList names() const { return m_names; }
    int selectedIndex() const { return m_index; }
    bool available() const { return !m_device.isNull(); }
    QAudioDevice device() const { return m_device; }
    Q_INVOKABLE void select(int index);

signals:
    void changed();
    void deviceChanged();

private:
    void refresh();
    QAudioDevice::Mode m_mode;
    QMediaDevices m_devices;
    QList<QAudioDevice> m_choices;
    QAudioDevice m_device;
    QStringList m_names;
    QString m_settingsKey;
    QByteArray m_selectedId;
    int m_index = 0;
};
