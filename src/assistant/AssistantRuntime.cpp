#include "AssistantRuntime.h"

#include <QQmlContext>
#include <utility>

AssistantRuntime::AssistantRuntime(QQmlContext &context, VehicleState &state, MediaState &music,
                                   MediaSession &session, VideoLibrary &videos, RadioService &radio, QAudioOutput &audio)
    : m_vehicle(state),
      m_media(music, session, videos, [this](const QString &request, bool spoken, AssistantMedia::Reply reply) {
          return m_vehicle.submit(request, std::move(reply), spoken);
      }, &radio),
      m_speech(audio), m_nlu(state, m_vehicle, m_media) {
    m_wake.setInputRoute(m_speech.route());
    m_voice.setInputRoute(m_speech.route());
    context.setContextProperty("Assistant", &m_media);
    context.setContextProperty("AssistantSpeech", &m_speech);
    context.setContextProperty("AssistantNlu", &m_nlu);
    context.setContextProperty("AssistantVoice", &m_voice);
    context.setContextProperty("AssistantAppearance", &m_appearance);
    context.setContextProperty("AssistantWake", &m_wake);
    QObject::connect(&m_speech, &AssistantSpeech::recognized, &m_nlu, [this](const QString &text) {
        if (assistantCancelsRequest(text))
            m_speech.cancel();
        m_nlu.submit(text, true);
    });
    QObject::connect(&m_media, &AssistantMedia::replyReady, &m_voice, &AssistantVoice::reply);
    QObject::connect(&m_voice, &AssistantVoice::transcriptionRequested, &m_speech, [this](const QUrl &url) {
        m_speech.cancel();
        m_speech.transcribe(url, "reference");
    });
    QObject::connect(&m_speech, &AssistantSpeech::dictated, &m_voice, [this](const QString &text, const QString &target) {
        if (target == "reference")
            m_voice.setTranscript(text);
        else if (target == "preview")
            m_voice.setPreviewText(text);
    });
    QObject::connect(m_voice.referenceClip(), &AssistantAudioClip::changed, &m_speech, [this] {
        const bool recording = m_voice.referenceClip()->recording();
        m_speech.setOtherRecording(recording);
        if (recording)
            m_voice.stop();
    });
    QObject::connect(&m_voice, &AssistantVoice::changed, &m_speech, [this] {
        m_nlu.setPromptActive(m_voice.busy() || m_voice.playing());
        m_speech.setReplyPlaying(m_voice.playing());
    });
    QObject::connect(&m_speech, &AssistantSpeech::changed, &m_voice, [this] {
        if (m_speech.recording())
            m_voice.stop();
    });
    m_wakeSync.setSingleShot(true);
    QObject::connect(&m_wakeSync, &QTimer::timeout, &m_wake, [this] { m_wake.setSuspended(false); });
    m_followUp.setSingleShot(true);
    m_wakeCapture.setSingleShot(true);
    const auto syncWake = [this] {
        const bool processing = m_speech.recording() || m_speech.recognizing() || m_nlu.busy() || m_media.pending()
            || m_voice.referenceClip()->busy() || m_voice.countdown() > 0;
        const bool quiet = !m_voice.busy() && !m_voice.playing();
        if (!processing && quiet && m_speech.wantsFollowUp()) {
            if (!m_followUp.isActive())
                m_followUp.start(450);
        } else {
            m_followUp.stop();
        }
        const bool allowInterrupt = m_voice.playing() && !m_voice.previewActive() && m_wake.bargeIn();
        const bool busy = m_wake.uiBlocked() || processing || m_wakeCapture.isActive()
            || m_followUp.isActive() || (!quiet && !allowInterrupt);
        if (busy) {
            m_wakeSync.stop();
            m_wake.setSuspended(true);
        } else {
            m_wakeSync.start(0);
        }
    };
    QObject::connect(&m_followUp, &QTimer::timeout, &m_speech, [this, syncWake] {
        if (m_speech.wantsFollowUp() && !m_voice.busy() && !m_voice.playing() && !m_nlu.busy() && !m_media.pending())
            m_speech.startAutomatic();
        syncWake();
    });
    QObject::connect(&m_wakeCapture, &QTimer::timeout, &m_speech, [this, syncWake] {
        if (m_speech.sessionOpen())
            m_speech.startAutomatic();
        syncWake();
    });
    QObject::connect(&m_speech, &AssistantSpeech::changed, &m_wake, syncWake);
    QObject::connect(&m_nlu, &AssistantNlu::changed, &m_wake, syncWake);
    QObject::connect(&m_media, &AssistantMedia::changed, &m_wake, syncWake);
    QObject::connect(&m_voice, &AssistantVoice::changed, &m_wake, syncWake);
    QObject::connect(&m_wake, &AssistantWake::changed, &m_speech, [this, syncWake] {
        QTimer::singleShot(0, &m_speech, syncWake);
    });
    QObject::connect(&m_speech, &AssistantSpeech::captureRequested, &m_wake, [this] {
        m_wakeSync.stop();
        m_wake.setSuspended(true);
    });
    QObject::connect(&m_wake, &AssistantWake::activated, &m_speech, [this, syncWake] {
        m_wakeCapture.start(240);
        m_voice.stop();
        if (!m_nlu.followUpActive() && m_media.choices().isEmpty())
            m_media.clearFeedback();
        syncWake();
    });
}
