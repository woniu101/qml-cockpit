#include "AssistantAudioRoute.h"

#include <QSettings>

AssistantAudioRoute::AssistantAudioRoute(QAudioDevice::Mode mode)
    : m_mode(mode), m_settingsKey(mode == QAudioDevice::Input ? "assistant/inputDevice" : "assistant/outputDevice") {
    if (!qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS"))
        m_selectedId = QSettings().value(m_settingsKey).toByteArray();
    connect(&m_devices, mode == QAudioDevice::Input ? &QMediaDevices::audioInputsChanged : &QMediaDevices::audioOutputsChanged,
            this, &AssistantAudioRoute::refresh);
    refresh();
}

void AssistantAudioRoute::select(int index) {
    if (index < 0 || index > m_choices.size())
        return;
    const QByteArray id = index == 0 ? QByteArray() : m_choices.at(index - 1).id();
    if (id == m_selectedId)
        return;
    m_selectedId = id;
    if (!qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS"))
        QSettings().setValue(m_settingsKey, id);
    refresh();
}

void AssistantAudioRoute::refresh() {
    const bool input = m_mode == QAudioDevice::Input;
    m_choices = input ? QMediaDevices::audioInputs() : QMediaDevices::audioOutputs();
    const auto defaultDevice = input ? QMediaDevices::defaultAudioInput() : QMediaDevices::defaultAudioOutput();
    m_names = {defaultDevice.isNull() ? QStringLiteral("Windows 默认 · 无可用设备")
                                    : QStringLiteral("Windows 默认 · %1").arg(defaultDevice.description())};
    QAudioDevice resolved;
    m_index = 0;
    for (const auto &device : m_choices) {
        m_names.append(device.description());
        if (device.id() == m_selectedId) {
            m_index = int(m_names.size()) - 1;
            resolved = device;
        }
    }
    if (m_selectedId.isEmpty()) {
        resolved = defaultDevice;
    } else if (m_index == 0) {
        m_index = int(m_names.size());
        m_names.append(QStringLiteral("已选设备 · 当前未连接"));
    }
    const bool changedDevice = m_device != resolved;
    m_device = resolved;
    if (changedDevice)
        emit deviceChanged();
    emit changed();
}
