#include "AssistantRuntime.h"

#include <QQmlContext>
#include <utility>

AssistantRuntime::AssistantRuntime(QQmlContext &context, VehicleState &state, MediaState &music,
                                   MediaSession &session, VideoLibrary &videos, RadioService &radio, QAudioOutput &audio)
    : m_vehicle(state),
      m_media(music, session, videos, [this](const QString &request, bool spoken, AssistantMedia::Reply reply) {
          return m_vehicle.submit(request, std::move(reply), spoken);
      }, &radio),
      m_speech(audio), m_voice(audio), m_nlu(state, m_vehicle, m_media) {
    context.setContextProperty("Assistant", &m_media);
    context.setContextProperty("AssistantSpeech", &m_speech);
    context.setContextProperty("AssistantNlu", &m_nlu);
    context.setContextProperty("AssistantVoice", &m_voice);
    context.setContextProperty("AssistantAppearance", &m_appearance);
    context.setContextProperty("AssistantWake", &m_wake);
    QObject::connect(&m_speech, &AssistantSpeech::recognized, &m_nlu, [this](const QString &text) {
        m_nlu.submit(text, true);
    });
    QObject::connect(&m_media, &AssistantMedia::spokenReply, &m_voice, &AssistantVoice::speak);
    QObject::connect(&m_voice, &AssistantVoice::changed, &m_speech, [this] {
        m_speech.setReplyPlaying(m_voice.playing());
    });
    QObject::connect(&m_speech, &AssistantSpeech::changed, &m_voice, [this] {
        if (m_speech.recording())
            m_voice.stop();
    });
    m_wakeSync.setSingleShot(true);
    QObject::connect(&m_wakeSync, &QTimer::timeout, &m_wake, [this] { m_wake.setSuspended(false); });
    const auto syncWake = [this] {
        const bool busy = m_speech.recording() || m_speech.recognizing() || m_nlu.busy() || m_media.pending()
            || m_voice.busy() || m_voice.playing();
        if (busy) {
            m_wakeSync.stop();
            m_wake.setSuspended(true);
        } else {
            m_wakeSync.start(0);
        }
    };
    QObject::connect(&m_speech, &AssistantSpeech::changed, &m_wake, syncWake);
    QObject::connect(&m_nlu, &AssistantNlu::changed, &m_wake, syncWake);
    QObject::connect(&m_media, &AssistantMedia::changed, &m_wake, syncWake);
    QObject::connect(&m_voice, &AssistantVoice::changed, &m_wake, syncWake);
    QObject::connect(&m_speech, &AssistantSpeech::captureRequested, &m_wake, [this] {
        m_wakeSync.stop();
        m_wake.setSuspended(true);
    });
    QObject::connect(&m_wake, &AssistantWake::activated, &m_speech, [this, syncWake] {
        m_media.clearFeedback();
        m_speech.start();
        syncWake();
    });
}
