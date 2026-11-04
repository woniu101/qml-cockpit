#pragma once

#include "AssistantWorker.h"

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QTemporaryDir>

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
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY preferencesChanged)
    Q_PROPERTY(int speed READ speed WRITE setSpeed NOTIFY preferencesChanged)
    Q_PROPERTY(bool rateAvailable READ rateAvailable CONSTANT)
    Q_PROPERTY(bool repliesEnabled READ repliesEnabled WRITE setRepliesEnabled NOTIFY preferencesChanged)
public:
    explicit AssistantVoice(QAudioOutput &mediaOutput);
    bool configured() const { return m_worker.configured() && m_dir.isValid(); }
    bool busy() const { return m_busy; }
    bool synthesizing() const { return m_busy && !m_discard; }
    bool playing() const { return m_player.playbackState() == QMediaPlayer::PlayingState; }
    bool previewActive() const { return m_preview && (synthesizing() || playing()); }
    bool verified() const { return !m_verifiedTranscript.isEmpty(); }
    QString referenceName() const { return m_draftName; }
    QString activeName() const { return m_activeName; }
    QString transcript() const { return m_draftTranscript; }
    QString message() const { return m_message; }
    int volume() const { return m_volume; }
    int speed() const { return m_speed; }
    bool rateAvailable() const;
    bool repliesEnabled() const { return m_repliesEnabled; }
    void setVolume(int value);
    void setSpeed(int value);
    void setRepliesEnabled(bool value);
    Q_INVOKABLE void resetPlaybackPreferences();
    Q_INVOKABLE void importReference(const QUrl &url);
    Q_INVOKABLE void preview(const QString &transcript);
    Q_INVOKABLE bool save(const QString &transcript);
    Q_INVOKABLE void stop();
    void speak(const QString &text);

signals:
    void changed();
    void preferencesChanged();

private:
    void synthesize(const QString &text, const QString &reference, const QString &transcript, bool preview);
    void complete(const QJsonObject &result);
    void status(const QString &message);
    void applyPlaybackPreferences();
    QAudioOutput &m_mediaOutput;
    QTemporaryDir m_dir;
    QAudioOutput m_output;
    QMediaPlayer m_player;
    AssistantWorker m_worker;
    QString m_storage, m_reference, m_referenceText, m_draft, m_draftName, m_activeName, m_draftTranscript,
        m_verifiedTranscript, m_pendingTranscript, m_wave, m_message, m_queuedReply;
    int m_serial = 0;
    int m_volume = 85, m_speed = 100;
    bool m_repliesEnabled = true;
    bool m_busy = false, m_preview = false, m_discard = false;
};
