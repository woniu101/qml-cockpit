#pragma once

#include "AssistantInference.h"
#include "AssistantAudioRoute.h"
#include "AssistantAudioClip.h"

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QTemporaryDir>
#include <QTimer>

class AssistantVoice final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool configured READ configured CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool synthesizing READ synthesizing NOTIFY changed)
    Q_PROPERTY(bool playing READ playing NOTIFY changed)
    Q_PROPERTY(bool previewActive READ previewActive NOTIFY changed)
    Q_PROPERTY(bool verified READ verified NOTIFY changed)
    Q_PROPERTY(QString referenceName READ referenceName NOTIFY changed)
    Q_PROPERTY(QString activeName READ activeName NOTIFY changed)
    Q_PROPERTY(QString transcript READ transcript NOTIFY changed)
    Q_PROPERTY(QString activeTranscript READ activeTranscript NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY preferencesChanged)
    Q_PROPERTY(int speed READ speed WRITE setSpeed NOTIFY preferencesChanged)
    Q_PROPERTY(bool rateAvailable READ rateAvailable CONSTANT)
    Q_PROPERTY(QString replyMode READ replyMode WRITE setReplyMode NOTIFY preferencesChanged)
    Q_PROPERTY(bool failed READ failed NOTIFY changed)
    Q_PROPERTY(AssistantAudioRoute* route READ route CONSTANT)
    Q_PROPERTY(AssistantAudioClip* referenceClip READ referenceClip CONSTANT)
    Q_PROPERTY(QString previewText READ previewText WRITE setPreviewText NOTIFY changed)
    Q_PROPERTY(QString readingPrompt READ readingPrompt CONSTANT)
    Q_PROPERTY(bool dirty READ dirty NOTIFY changed)
    Q_PROPERTY(int countdown READ countdown NOTIFY changed)
public:
    explicit AssistantVoice(QString storage = {});
    AssistantAudioRoute *route() { return &m_route; }
    AssistantAudioClip *referenceClip() { return &m_clip; }
    QString previewText() const { return m_previewText; }
    QString readingPrompt() const { return QStringLiteral("你好，欢迎来到我的座舱。窗外的风景很美，让我们听着音乐，轻松地出发吧。"); }
    bool dirty() const { return m_dirty; }
    int countdown() const { return m_countdown; }
    void setInputRoute(AssistantAudioRoute *route);
    bool configured() const { return m_worker.configured() && m_dir.isValid(); }
    bool busy() const { return m_busy; }
    bool synthesizing() const { return m_busy; }
    bool playing() const { return m_player.playbackState() == QMediaPlayer::PlayingState; }
    bool previewActive() const { return m_preview && (synthesizing() || playing()); }
    bool verified() const { return !m_verifiedTranscript.isEmpty(); }
    QString referenceName() const { return m_draftName; }
    QString activeName() const { return m_activeName; }
    QString transcript() const { return m_draftTranscript; }
    QString activeTranscript() const { return m_referenceText; }
    QString message() const { return m_message; }
    int volume() const { return m_volume; }
    int speed() const { return m_speed; }
    bool rateAvailable() const;
    QString replyMode() const { return m_replyMode; }
    bool failed() const { return m_failed; }
    void setVolume(int value);
    void setSpeed(int value);
    void setReplyMode(const QString &value);
    void reply(const QString &text, bool voiceInput, bool announce = true);
    QUrl wakePrompt() const { return m_wakePrompt; }
    void setWakePromptEnabled(bool enabled);
    Q_INVOKABLE void resetPlaybackPreferences();
    Q_INVOKABLE void importReference(const QUrl &url);
    Q_INVOKABLE void preview();
    Q_INVOKABLE void setTranscript(const QString &text);
    void setPreviewText(const QString &text);
    Q_INVOKABLE void recordReference();
    Q_INVOKABLE void cancelRecording();
    Q_INVOKABLE void playReference(bool active = false);
    Q_INVOKABLE void discardDraft();
    Q_INVOKABLE void trimReference(double start, double length);
    Q_INVOKABLE bool save(const QString &transcript);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void speak(const QString &text);

signals:
    void changed();
    void preferencesChanged();
    void transcriptionRequested(const QUrl &wave);
    void wakePromptChanged();

private:
    void synthesize(const QString &text, const QString &reference, const QString &transcript, bool preview);
    void complete(const QJsonObject &result);
    void status(const QString &message);
    void applyPlaybackPreferences();
    void prepareWakePrompt();
    void cancelWakePromptPreparation();
    AssistantAudioRoute m_route{QAudioDevice::Output};
    AssistantAudioRoute *m_inputRoute = nullptr;
    AssistantAudioClip m_clip;
    QTimer m_countdownTimer;
    QTemporaryDir m_dir;
    QAudioOutput m_output;
    QMediaPlayer m_player;
    AssistantInference m_worker;
    QString m_storage, m_reference, m_referenceText, m_draft, m_draftName, m_activeName, m_draftTranscript,
        m_verifiedTranscript, m_pendingTranscript, m_wave, m_message;
    int m_serial = 0;
    int m_volume = 85, m_speed = 100;
    bool m_busy = false, m_preview = false;
    QString m_replyMode = "all";
    bool m_failed = false;
    bool m_dirty = false, m_reading = false;
    int m_countdown = 0;
    QString m_previewText = QStringLiteral("右后车窗已调到百分之三十。");
    QUrl m_wakePrompt;
    QString m_wakePromptFile;
    bool m_wakePromptEnabled = false;
    bool m_wakePromptPreparing = false;
    bool m_wakePromptAttempted = false;
};
