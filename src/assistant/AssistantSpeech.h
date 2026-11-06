#pragma once

#include <QBuffer>
#include "AssistantAudioRoute.h"
#include "AssistantEndpoint.h"
#include "AssistantAudioClip.h"
#include <QObject>
#include "AssistantInference.h"
#include <QTimer>
#include <memory>

class QAudioOutput;
class QAudioSource;
class QUrl;

class AssistantSpeech final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool configured READ configured CONSTANT)
    Q_PROPERTY(bool inputAvailable READ inputAvailable NOTIFY changed)
    Q_PROPERTY(AssistantAudioRoute* route READ route CONSTANT)
    Q_PROPERTY(bool recording READ recording NOTIFY changed)
    Q_PROPERTY(bool recognizing READ recognizing NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(bool continuous READ continuous WRITE setContinuous NOTIFY changed)
    Q_PROPERTY(bool automatic READ automatic NOTIFY changed)
    Q_PROPERTY(bool dictating READ dictating NOTIFY changed)
    Q_PROPERTY(double level READ level NOTIFY changed)
    Q_PROPERTY(bool failed READ failed NOTIFY changed)
public:
    explicit AssistantSpeech(QAudioOutput &);
    ~AssistantSpeech() override;
    bool configured() const;
    bool inputAvailable() const;
    AssistantAudioRoute *route() { return &m_route; }
    bool recording() const { return m_state == State::Recording; }
    bool recognizing() const { return m_state == State::Recognizing || m_clip.busy(); }
    bool dictating() const { return m_dictating; }
    double level() const { return m_level; }
    bool failed() const { return m_failed; }
    QString message() const { return m_message; }
    bool continuous() const { return m_continuous; }
    bool automatic() const { return recording() && m_automatic; }
    bool sessionOpen() const { return m_sessionOpen; }
    bool wantsFollowUp() const { return m_followUpWanted && m_sessionOpen && m_continuous && !recording() && !recognizing(); }
    void setContinuous(bool value);
    void startAutomatic();
    Q_INVOKABLE void setSessionOpen(bool open);
    Q_INVOKABLE void start();
    Q_INVOKABLE void finish();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void openWave(const QUrl &url);
    Q_INVOKABLE void transcribe(const QUrl &url, const QString &target = "text");
    Q_INVOKABLE void dictate(const QString &target = "text");
    void setOtherRecording(bool value) { m_otherRecording = value; updateAudioFocus(); }
    void setReplyPlaying(bool playing);

signals:
    void captureRequested();
    void changed();
    void recognized(const QString &text);
    void dictated(const QString &text, const QString &target);

private:
    enum class State { Idle, Recording, Recognizing };
    void recognize(QByteArray audio, const QStringList &format);
    void startCapture(bool automatic);
    void checkEndpoint();
    void stopCapture();
    void updateAudioFocus();
    void fail(const QString &message);
    QAudioOutput &m_mediaOutput;
    AssistantAudioClip m_clip{15, 0.2, 15};
    AssistantAudioRoute m_route{QAudioDevice::Input};
    QBuffer m_buffer;
    std::unique_ptr<QAudioSource> m_source;
    AssistantInference m_asr;
    QTimer m_limit;
    QTimer m_endpointTimer;
    QAudioFormat m_captureFormat;
    AssistantEndpoint m_endpoint;
    qsizetype m_endpointOffset = 0;
    QString m_model, m_message;
    QStringList m_format;
    State m_state = State::Idle;
    bool m_restoreMute = false;
    bool m_replyPlaying = false;
    bool m_continuous = false, m_automatic = false, m_sessionOpen = false, m_fromMicrophone = false, m_followUpWanted = false;
    bool m_dictating = false;
    double m_level = 0;
    QString m_dictationTarget;
    bool m_otherRecording = false;
    bool m_failed = false;
};
