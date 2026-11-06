#pragma once
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QSet>
#include <QTimer>
#include <QStringList>

// Media-domain state; no vehicle or Windows device dependency.
class MediaState : public QObject {
    Q_OBJECT
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY changed)
    Q_PROPERTY(bool playing READ playing WRITE setPlaying NOTIFY changed)
    Q_PROPERTY(int track READ track NOTIFY changed)
    Q_PROPERTY(QString trackTitle READ trackTitle NOTIFY changed)
    Q_PROPERTY(QString trackUrl READ trackUrl NOTIFY changed)
    Q_PROPERTY(QVariantList tracks READ tracks NOTIFY changed)
    Q_PROPERTY(QString playMode READ playMode WRITE setPlayMode NOTIFY changed)
    Q_PROPERTY(QString mediaStatus READ mediaStatus NOTIFY changed)
    Q_PROPERTY(QString mediaError READ mediaError NOTIFY changed)
    Q_PROPERTY(double position READ position NOTIFY positionChanged)
    Q_PROPERTY(double duration READ duration NOTIFY positionChanged)
  public:
    explicit MediaState(QObject *parent = nullptr);
    ~MediaState() override;
    int volume() const { return m_volume; }
    bool playing() const { return m_playing; }
    int track() const { return m_track; }
    QString trackTitle() const;
    QString trackUrl() const;
    QVariantList tracks() const { return m_tracks; }
    QString playMode() const { return m_playMode; }
    QString mediaStatus() const { return m_mediaStatus; }
    QString mediaError() const { return m_mediaError; }
    double position() const { return m_position; }
    double duration() const { return m_duration; }
    void setVolume(int);
    void setPlaying(bool);
    void setPlayMode(const QString &);
    void updatePosition(double);
    void updateDuration(double);
    void updateMediaStatus(const QString &, const QString &error = QString());
    bool demoActive() const { return m_demoActive; }
    void mergeMusicTracks(const QVariantList &);
    void updateTrackProperties(const QString &url, const QVariantMap &properties);
    void restoreMusicPosition(const QString &url, qint64 position);
    void removeMusicTrack(int index);
    QStringList playQueue() const;
    QString queueLabel() const { return m_queueLabel; }
    bool hasExplicitQueue() const { return m_explicitQueue; }
    void setPlayQueue(const QStringList &urls, const QString &label);
    void replaceMusicTrack(int index, const QVariantMap &row);
    Q_INVOKABLE void moveQueueTrack(const QString &url, int direction);
    Q_INVOKABLE void removeQueueTrack(const QString &url);
    Q_INVOKABLE void nextTrack(int direction = 1);
    Q_INVOKABLE void selectTrack(int index);
    Q_INVOKABLE void finishTrack();
    Q_INVOKABLE void importTracks(const QVariantList &urls);
    Q_INVOKABLE void retryTrack();
    Q_INVOKABLE void seek(double milliseconds);
    Q_INVOKABLE QVariantMap snapshot() const;
    Q_INVOKABLE void restoreSnapshot(const QVariantMap &);
    Q_INVOKABLE void resetRuntime();
    Q_INVOKABLE void setDemoActive(bool);
  signals:
    void changed();
    void positionChanged();
    void queueChanged();
    void toast(const QString &, const QString &kind = QStringLiteral("success"));
    void seekRequested(qint64);
    void mediaReloadRequested();

  private:
    void schedulePreferences();
    void savePreferences();
    int musicIndex(const QString &url) const;
    bool musicAvailable(int index) const;
    void activateMusic(int index, bool recordHistory);
    void resetMusicHistory();
    QVariantList m_tracks;
    QStringList m_queueUrls, m_musicHistory;
    QSet<QString> m_shuffleVisited;
    QString m_queueLabel = QStringLiteral("全部歌曲");
    int m_historyCursor = -1;
    bool m_explicitQueue = false;
    QString m_playMode = "all", m_mediaStatus = "loading", m_mediaError;
    double m_position = 0, m_duration = 0;
    int m_volume = 45, m_track = 0;
    bool m_playing = false, m_demoActive = false, m_preferencesEnabled = false;
    QString m_settingsPath;
    QTimer m_saveTimer;
};
