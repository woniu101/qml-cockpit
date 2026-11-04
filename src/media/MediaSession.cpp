#include "MediaSession.h"
#include "MediaState.h"
#include "../MusicSource.h"
#include <QAudioOutput>
#include <QFileInfo>
#include <QMediaMetaData>
#include <QScopedValueRollback>
#include <QVideoSink>
#include <cmath>

MediaSession::MediaSession(MediaState *music, QAudioOutput *audio, QObject *parent, bool muted)
    : QObject(parent), m_music(music), m_audio(audio), m_muted(muted) {
    m_player.setAudioOutput(audio);
    connect(music, &MediaState::changed, this, &MediaSession::synchronizeMusic);
    connect(music, &MediaState::seekRequested, this, [this](qint64 at) {
        if (m_source == Source::Music) {
            m_pendingSeek = at;
            applyPendingSeek();
        }
    });
    connect(music, &MediaState::mediaReloadRequested, this, [this] {
        if (m_source == Source::Music) {
            clearPlayer();
            synchronizeMusic();
        }
    });
    connect(&m_player, &QMediaPlayer::positionChanged, this, [this](qint64 at) {
        if (m_syncing)
            return;
        if (m_source == Source::Music)
            m_music->updatePosition(at);
        checkpoint();
        emit positionChanged();
    });
    connect(&m_player, &QMediaPlayer::durationChanged, this, [this](qint64 value) {
        if (m_syncing)
            return;
        if (m_source == Source::Music)
            m_music->updateDuration(value);
        applyPendingSeek();
        emit positionChanged();
    });
    connect(&m_player, &QMediaPlayer::seekableChanged, this, [this] {
        applyPendingSeek();
        emit changed();
    });
    connect(&m_player, &QMediaPlayer::tracksChanged, this, &MediaSession::changed);
    connect(&m_player, &QMediaPlayer::hasAudioChanged, this, [this] {
        outputGuard();
        emit changed();
    });
    connect(&m_player, &QMediaPlayer::playbackStateChanged, this, &MediaSession::changed);
    connect(&m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (m_syncing)
            return;
        if (status == QMediaPlayer::EndOfMedia) {
            if (m_source == Source::Music && m_music->playing())
                m_music->finishTrack();
            else if (m_source == Source::Video) {
                checkpoint();
                m_phase = "completed";
                m_fullscreen = false;
                emit changed();
            } else if (m_source == Source::Radio)
                emit radioInterrupted(m_generation, QStringLiteral("直播连接已结束"));
            return;
        }
        if (status == QMediaPlayer::LoadingMedia || status == QMediaPlayer::BufferingMedia ||
            status == QMediaPlayer::StalledMedia) {
            m_phase = m_source == Source::Video && !m_videoAllowed ? "blocked"
                      : status == QMediaPlayer::LoadingMedia       ? "connecting"
                                                                   : "buffering";
            if (m_source == Source::Music)
                m_music->updateMediaStatus("loading");
            if (m_source == Source::Radio && !m_liveWatchdog.isActive())
                m_liveWatchdog.start();
        }
        if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) {
            if (m_source == Source::Video && m_player.videoTracks().isEmpty()) {
                m_player.pause();
                m_error = QStringLiteral("文件不包含可播放的视频轨道");
                m_phase = "error";
                emit changed();
                return;
            }
            if (m_source == Source::Video && !m_videoAllowed)
                m_phase = "blocked";
            else
                m_phase = playing() ? "playing" : "paused";
            if (m_source == Source::Music)
                m_music->updateMediaStatus("ready");
            if (status == QMediaPlayer::BufferedMedia)
                m_liveWatchdog.stop();
            applyPendingSeek();
            outputGuard();
        }
        emit changed();
    });
    connect(&m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString &message) {
                if (m_syncing)
                    return;
                if (m_source == Source::Radio) {
                    emit radioInterrupted(m_generation, QStringLiteral("电台暂时无法播放"));
                    return;
                }
                m_error = message;
                m_phase = "error";
                if (m_source == Source::Music)
                    m_music->updateMediaStatus("error", message);
                emit notice(m_source == Source::Video ? QStringLiteral("视频暂时无法播放，可重试或返回视频库")
                                                      : QStringLiteral("音源加载失败，可在音乐页重试"));
                emit changed();
            });
    m_liveWatchdog.setSingleShot(true);
    m_liveWatchdog.setInterval(12000);
    connect(&m_liveWatchdog, &QTimer::timeout, this, [this] {
        if (m_source == Source::Radio)
            emit radioInterrupted(m_generation, QStringLiteral("电台连接超时"));
    });
}
MediaSession::~MediaSession() {
    checkpoint();
    // Member QObject signals must not call back after the session's value members are destroyed.
    disconnect(&m_player, nullptr, this, nullptr);
    disconnect(m_music, nullptr, this, nullptr);
    clearPlayer();
    m_player.setVideoOutput(nullptr);
    m_player.setAudioBufferOutput(nullptr);
    m_player.setAudioOutput(nullptr);
}
QString MediaSession::kind() const {
    return m_source == Source::Music   ? "music"
           : m_source == Source::Radio ? "radio"
           : m_source == Source::Video ? "video"
                                       : "phone";
}
QString MediaSession::title() const { return m_source == Source::Music ? m_music->trackTitle() : m_title; }
void MediaSession::clearPlayer() {
    QScopedValueRollback<bool> guard(m_syncing, true);
    m_liveWatchdog.stop();
    m_player.stop();
    m_player.setSource(QUrl());
    m_device.reset();
    m_pendingSeek = -1;
    ++m_seekTicket;
    m_seekScheduled = false;
    m_player.setPlaybackRate(1.0);
    emit positionChanged();
}
void MediaSession::replace(Source source) {
    checkpoint();
    {
        QScopedValueRollback<bool> guard(m_syncing, true);
        if (m_source == Source::Music)
            m_music->setPlaying(false);
        clearPlayer();
    }
    m_source = source;
    m_error.clear();
    m_phase = "paused";
    m_fullscreen = false;
    ++m_generation;
    emit sessionChanged();
}
void MediaSession::synchronizeMusic() {
    m_audio->setVolume(m_muted ? 0.0f : m_music->volume() / 100.0f);
    if (m_syncing)
        return;
    if (m_music->playing() && m_phoneActive) {
        m_music->setPlaying(false);
        return;
    }
    if (m_source != Source::Music) {
        if (!m_music->playing()) {
            emit changed();
            return;
        }
        replace(Source::Music);
        m_music->setPlaying(true);
    }
    if (m_source != Source::Music)
        return;
    if (m_music->playing() && !outputGuard()) {
        m_music->setPlaying(false);
        return;
    }
    const QUrl url(m_music->trackUrl());
    if (m_player.source() != url) {
        m_pendingSeek = qint64(m_music->position());
        setMusicSource(m_player, url, m_device);
    }
    if (m_music->playing())
        m_player.play();
    else
        m_player.pause();
    if (!m_music->playing() && m_phase != "error")
        m_phase = "paused";
    emit changed();
}
void MediaSession::applyPendingSeek() {
    if (m_pendingSeek < 0 || m_seekScheduled || !m_player.isSeekable() || m_player.duration() <= 0)
        return;
    if (m_player.mediaStatus() != QMediaPlayer::LoadedMedia &&
        m_player.mediaStatus() != QMediaPlayer::BufferedMedia)
        return;
    m_seekScheduled = true;
    const auto token = m_generation;
    const auto seekTicket = ++m_seekTicket;
    const auto source = m_player.source();
    QTimer::singleShot(0, this, [this, token, seekTicket, source] {
        if (seekTicket != m_seekTicket)
            return;
        m_seekScheduled = false;
        if (token != m_generation || source != m_player.source()) {
            applyPendingSeek();
            return;
        }
        if (m_pendingSeek < 0)
            return;
        const auto at = qMin(m_pendingSeek, qMax(0LL, m_player.duration() - 1000));
        m_pendingSeek = -1;
        m_player.setPosition(at);
    });
}
bool MediaSession::outputGuard() {
    if (m_outputAvailable || m_source == Source::Phone || (m_source == Source::Video && !m_player.hasAudio()))
        return true;
    m_player.pause();
    if (m_source == Source::Music)
        m_music->setPlaying(false);
    m_phase = "paused";
    emit notice(QStringLiteral("没有可用音频输出，请连接扬声器或耳机"));
    emit changed();
    return false;
}
void MediaSession::useMusic() {
    if (m_phoneActive)
        return;
    if (m_source != Source::Music)
        replace(Source::Music);
    synchronizeMusic();
}
quint64 MediaSession::beginRadio(const QString &id, const QString &name) {
    if (m_phoneActive || !m_outputAvailable) {
        emit notice(QStringLiteral("请先断开手机音乐并连接音频输出"));
        return 0;
    }
    replace(Source::Radio);
    m_id = id;
    m_title = name;
    m_phase = "resolving";
    emit changed();
    return m_generation;
}
void MediaSession::openRadio(quint64 token, const QUrl &url) {
    if (token != m_generation || m_source != Source::Radio)
        return;
    clearPlayer();
    m_phase = "connecting";
    m_player.setSource(url);
    m_player.play();
    m_liveWatchdog.start();
    emit changed();
}
void MediaSession::radioFailure(quint64 token, const QString &message, bool retrying) {
    if (token != m_generation || m_source != Source::Radio)
        return;
    clearPlayer();
    m_error = message;
    m_phase = retrying ? "reconnecting" : "error";
    emit changed();
}
void MediaSession::playVideo(const QUrl &url, const QString &name, qint64 bookmark) {
    if (!url.isLocalFile() || !QFileInfo(url.toLocalFile()).isFile()) {
        emit notice(QStringLiteral("视频文件不可用，请重新定位或选择其他视频"));
        return;
    }
    if (m_phoneActive) {
        emit notice(QStringLiteral("请先断开手机音乐，再播放本地视频"));
        return;
    }
    replace(Source::Video);
    m_id = url.toString();
    m_title = name;
    m_pendingSeek = qMax(0LL, bookmark);
    m_phase = m_videoAllowed ? "connecting" : "blocked";
    m_player.setVideoOutput(m_videoOutput);
    m_player.setSource(url);
    if (m_videoAllowed)
        m_player.play();
    emit changed();
}
void MediaSession::toggle() {
    if (m_source == Source::Phone)
        return;
    if (m_source == Source::Music) {
        m_music->setPlaying(!m_music->playing());
        return;
    }
    if (m_source == Source::Radio) {
        stop();
        return;
    }
    if (!m_videoAllowed) {
        m_phase = "blocked";
        emit changed();
        return;
    }
    if (playing())
        pause();
    else if (outputGuard()) {
        if (m_phase == "error") {
            const QUrl url(m_id);
            const auto name = m_title;
            playVideo(url, name, qint64(position()));
            return;
        }
        m_phase = "playing";
        m_player.play();
        emit changed();
    }
}
void MediaSession::pause() {
    if (m_source == Source::Music) {
        m_music->setPlaying(false);
        return;
    }
    if (m_source == Source::Radio) {
        stop();
        return;
    }
    m_player.pause();
    checkpoint();
    if (m_phase != "blocked")
        m_phase = "paused";
    emit changed();
}
void MediaSession::stop() {
    checkpoint();
    if (m_source == Source::Music)
        m_music->setPlaying(false);
    clearPlayer();
    m_phase = "stopped";
    m_fullscreen = false;
    ++m_generation;
    emit sessionChanged();
    emit changed();
}
void MediaSession::seek(double value) {
    if (canSeek() && std::isfinite(value))
        m_player.setPosition(qBound(0LL, qint64(value), m_player.duration()));
}
void MediaSession::setVideoOutput(QObject *sink) {
    if (m_videoOutput == sink)
        return;
    m_videoOutput = sink;
    if (m_source == Source::Video)
        m_player.setVideoOutput(sink);
    emit changed();
}
void MediaSession::setRate(double value) {
    if (m_source != Source::Video || !std::isfinite(value))
        return;
    m_player.setPlaybackRate(qBound(0.5, value, 2.0));
    emit changed();
}
void MediaSession::setFullscreen(bool value) {
    value = value && m_source == Source::Video && m_videoAllowed;
    if (m_fullscreen == value)
        return;
    m_fullscreen = value;
    emit changed();
}
void MediaSession::setVideoAllowed(bool value) {
    if (m_videoAllowed == value)
        return;
    m_videoAllowed = value;
    if (!value && m_source == Source::Video) {
        m_player.pause();
        checkpoint();
        m_phase = "blocked";
        m_fullscreen = false;
        emit notice(QStringLiteral("请在驻车后观看视频，当前进度已保留"));
    } else if (value && m_phase == "blocked")
        m_phase = "paused";
    emit changed();
}
void MediaSession::setOutputAvailable(bool value) {
    if (m_outputAvailable == value)
        return;
    m_outputAvailable = value;
    if (!value) {
        if (m_source == Source::Radio)
            stop();
        else
            outputGuard();
    }
    emit changed();
}
void MediaSession::setPhoneActive(bool value) {
    if (m_phoneActive == value)
        return;
    m_phoneActive = value;
    if (value) {
        replace(Source::Phone);
        m_title = QStringLiteral("手机蓝牙音乐");
        m_phase = "phone";
    } else if (m_source == Source::Phone) {
        replace(Source::Music);
        synchronizeMusic();
    }
    emit changed();
}
void MediaSession::videoPageLeft() {
    if (m_source == Source::Video) {
        pause();
        setFullscreen(false);
    }
}
QVariantMap MediaSession::snapshot() const {
    return {{"kind", kind()},
            {"id", m_id},
            {"title", m_title},
            {"position", position()},
            {"active", playing() || busy()}};
}
void MediaSession::restoreSnapshot(const QVariantMap &saved) {
    if (m_phoneActive)
        return;
    const auto kind = saved.value("kind").toString();
    if (kind == "radio") {
        if (saved.value("active").toBool())
            emit radioRestoreRequested(saved.value("id").toString());
        else if (beginRadio(saved.value("id").toString(), saved.value("title").toString()))
            stop();
    } else if (kind == "video") {
        playVideo(QUrl(saved.value("id").toString()), saved.value("title").toString(),
                  saved.value("position").toLongLong());
        if (!saved.value("active").toBool())
            pause();
    } else {
        useMusic();
    }
}
void MediaSession::checkpoint() {
    if (m_source == Source::Video && m_pendingSeek < 0 && m_player.duration() > 0)
        emit videoCheckpoint(QUrl(m_id), m_player.position(), m_player.duration());
}
static QVariantList trackList(const QList<QMediaMetaData> &tracks) {
    QVariantList result;
    for (int i = 0; i < tracks.size(); ++i) {
        const auto name = tracks[i].stringValue(QMediaMetaData::Language);
        result << QVariantMap{{"index", i},
                              {"label", name.isEmpty() ? QStringLiteral("轨道 %1").arg(i + 1) : name}};
    }
    return result;
}
QVariantList MediaSession::audioTracks() const { return trackList(m_player.audioTracks()); }
QVariantList MediaSession::subtitleTracks() const {
    auto rows = trackList(m_player.subtitleTracks());
    rows.prepend(QVariantMap{{"index", -1}, {"label", QStringLiteral("关闭字幕")}});
    return rows;
}
void MediaSession::setAudioTrack(int index) {
    if (index >= 0 && index < m_player.audioTracks().size())
        m_player.setActiveAudioTrack(index);
    emit changed();
}
void MediaSession::setSubtitleTrack(int index) {
    if (index >= -1 && index < m_player.subtitleTracks().size())
        m_player.setActiveSubtitleTrack(index);
    emit changed();
}
