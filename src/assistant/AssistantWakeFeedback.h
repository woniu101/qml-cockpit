#pragma once

#include "AssistantWake.h"

#include <QSoundEffect>
#include <QTimer>

class AssistantVoice;

class AssistantWakeFeedback final : public QObject {
    Q_OBJECT
public:
    explicit AssistantWakeFeedback(AssistantVoice &voice);
    bool play(AssistantWake::Feedback feedback);
    void cancel();

signals:
    void finished();

private:
    void complete();
    AssistantVoice &m_voice;
    QSoundEffect m_tone;
    QSoundEffect m_spoken;
    QSoundEffect *m_playing = nullptr;
    QTimer m_deadline;
};
