#include "AssistantWakeFeedback.h"
#include "AssistantVoice.h"

AssistantWakeFeedback::AssistantWakeFeedback(AssistantVoice &voice) : m_voice(voice) {
    m_tone.setSource(QUrl("qrc:/assistant/wake-tone.wav"));
    const auto updateDevice = [this] {
        const auto device = m_voice.route()->device();
        m_tone.setAudioDevice(device);
        m_spoken.setAudioDevice(device);
        if (m_playing)
            complete();
    };
    updateDevice();
    connect(m_voice.route(), &AssistantAudioRoute::deviceChanged, this, updateDevice);
    connect(&m_voice, &AssistantVoice::wakePromptChanged, this, [this] {
        m_spoken.setSource(m_voice.wakePrompt());
    });
    for (auto *effect : {&m_tone, &m_spoken}) {
        connect(effect, &QSoundEffect::playingChanged, this, [this, effect] {
            if (m_playing == effect && !effect->isPlaying())
                complete();
        }, Qt::QueuedConnection);
    }
    m_deadline.setSingleShot(true);
    connect(&m_deadline, &QTimer::timeout, this, &AssistantWakeFeedback::complete);
}

bool AssistantWakeFeedback::play(AssistantWake::Feedback feedback) {
    if (feedback == AssistantWake::Silent || !m_voice.route()->available() || m_voice.volume() == 0)
        return false;
    auto *effect = feedback == AssistantWake::Spoken && m_spoken.isLoaded() ? &m_spoken : &m_tone;
    if (!effect->isLoaded())
        return false;
    effect->setVolume(m_voice.volume() / 100.0f * (effect == &m_tone ? 0.55f : 1.0f));
    effect->play();
    if (!effect->isPlaying())
        return false;
    m_playing = effect;
    m_deadline.start(effect == &m_tone ? 350 : 2200);
    return true;
}

void AssistantWakeFeedback::cancel() {
    m_deadline.stop();
    auto *effect = m_playing;
    m_playing = nullptr;
    if (effect)
        effect->stop();
}

void AssistantWakeFeedback::complete() {
    cancel();
    emit finished();
}
