#pragma once
#include <QObject>
#include <QMediaPlayer>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <memory>
class MediaState;
class QAudioOutput;
class QIODevice;

// Owns playback, source switching and capabilities. Libraries never own players.
class MediaSession : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString kind READ kind NOTIFY changed)
    Q_PROPERTY(QString itemId READ itemId NOTIFY changed)
    Q_PROPERTY(QString title READ title NOTIFY changed)
    Q_PROPERTY(QString phase READ phase NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(bool playing READ playing NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool canSeek READ canSeek NOTIFY changed)
    Q_PROPERTY(bool videoAllowed READ videoAllowed NOTIFY changed)
    Q_PROPERTY(bool fullscreen READ fullscreen WRITE setFullscreen NOTIFY changed)
    Q_PROPERTY(bool hasAudio READ hasAudio NOTIFY changed)
    Q_PROPERTY(double position READ position NOTIFY positionChanged)
    Q_PROPERTY(double duration READ duration NOTIFY positionChanged)
    Q_PROPERTY(double rate READ rate WRITE setRate NOTIFY changed)
    Q_PROPERTY(QObject *videoOutput READ videoOutput WRITE setVideoOutput NOTIFY changed)
    Q_PROPERTY(QVariantList audioTracks READ audioTracks NOTIFY changed)
    Q_PROPERTY(QVariantList subtitleTracks READ subtitleTracks NOTIFY changed)
    Q_PROPERTY(int audioTrack READ audioTrack WRITE setAudioTrack NOTIFY changed)
    Q_PROPERTY(int subtitleTrack READ subtitleTrack WRITE setSubtitleTrack NOTIFY changed)
  public:
    enum class Source { Music, Radio, Video, Phone };
    Q_ENUM(Source)
    MediaSession(MediaState *, QAudioOutput *, QObject *parent = nullptr, bool muted = false);
    ~MediaSession() override;
    QMediaPlayer *player() { return &m_player; }
    QString kind() const;
    QString itemId() const { return m_id; }
    QString title() const;
    QString phase() const { return m_phase; }
    QString error() const { return m_error; }
    bool playing() const {
        return m_player.playbackState() == QMediaPlayer::PlayingState && m_phase != "blocked" &&
               m_source != Source::Phone;
    }
    bool busy() const {
        return QStringList{"connecting", "buffering", "resolving", "reconnecting"}.contains(m_phase);
    }
    bool canSeek() const {
        return m_source != Source::Radio && m_source != Source::Phone && m_player.isSeekable();
    }
    bool videoAllowed() const { return m_videoAllowed; }
    bool outputAvailable() const { return m_outputAvailable; }
    bool fullscreen() const { return m_fullscreen; }
    bool hasAudio() const { return m_player.hasAudio(); }
    double position() const { return m_source == Source::Radio ? 0 : m_player.position(); }
    double duration() const { return m_source == Source::Radio ? 0 : m_player.duration(); }
    double rate() const { return m_player.playbackRate(); }
    QObject *videoOutput() const { return m_videoOutput; }
    QVariantList audioTracks() const;
    QVariantList subtitleTracks() const;
    int audioTrack() const { return m_player.activeAudioTrack(); }
    int subtitleTrack() const { return m_player.activeSubtitleTrack(); }
    void setVideoOutput(QObject *);
    void setRate(double);
    void setAudioTrack(int);
    void setSubtitleTrack(int);
    void setFullscreen(bool);
    void setVideoAllowed(bool);
    void setOutputAvailable(bool);
    void setPhoneActive(bool);
    quint64 generation() const { return m_generation; }
    quint64 beginRadio(const QString &id, const QString &title);
    void openRadio(quint64, const QUrl &);
    void radioFailure(quint64, const QString &, bool retrying = false);
    Q_INVOKABLE void useMusic();
    Q_INVOKABLE void playVideo(const QUrl &, const QString &, qint64 bookmark = 0);
    Q_INVOKABLE void toggle();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void seek(double);
    Q_INVOKABLE void videoPageLeft();
    Q_INVOKABLE QVariantMap snapshot() const;
    Q_INVOKABLE void restoreSnapshot(const QVariantMap &);
  signals:
    void changed();
    void positionChanged();
    void sessionChanged();
    void notice(const QString &);
    void radioInterrupted(quint64 generation, const QString &);
    void radioRestoreRequested(const QString &id);
    void videoCheckpoint(const QUrl &, qint64 position, qint64 duration);

  private:
    void synchronizeMusic();
    void replace(Source);
    void clearPlayer();
    void applyPendingSeek();
    bool outputGuard();
    void checkpoint();
    MediaState *m_music;
    QAudioOutput *m_audio;
    QMediaPlayer m_player;
    QPointer<QObject> m_videoOutput;
    std::unique_ptr<QIODevice> m_device;
    Source m_source = Source::Music;
    QString m_id, m_title, m_phase = "paused", m_error;
    quint64 m_generation = 0, m_seekTicket = 0;
    qint64 m_pendingSeek = -1;
    bool m_seekScheduled = false, m_syncing = false, m_outputAvailable = true, m_phoneActive = false,
         m_videoAllowed = true, m_fullscreen = false, m_muted = false;
    QTimer m_liveWatchdog;
};
