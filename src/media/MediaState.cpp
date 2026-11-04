#include "MediaState.h"
#include <QSettings>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <algorithm>
#include <cmath>

MediaState::MediaState(QObject *parent) : QObject(parent) {
    for (int i = 0; i < 3; ++i)
        m_tracks.append(QVariantMap{
            {"title", QStringList{QStringLiteral("沿途"), QStringLiteral("晚风"), QStringLiteral("山间")}[i]},
            {"url", QString("qrc:/ui/audio/track%1.wav").arg(i)},
            {"source", QStringLiteral("本地演示音源")}});
    m_preferencesEnabled = !qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS");
    m_settingsPath =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/media-preferences.ini";
    if (const auto explicitFile = qEnvironmentVariable("CAR_TERMINAL_MEDIA_SETTINGS_FILE");
        !explicitFile.isEmpty())
        m_settingsPath = explicitFile;
    if (m_preferencesEnabled) {
        QSettings prefs(m_settingsPath, QSettings::IniFormat);
        if (!prefs.contains("version")) {
            const auto oldFile = qEnvironmentVariable(
                "CAR_TERMINAL_SETTINGS_FILE",
                QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/preferences.ini");
            QSettings previous(oldFile, QSettings::IniFormat);
            prefs.setValue("volume", previous.value("volume", 45));
            prefs.setValue("playMode", previous.value("playMode", "all"));
            prefs.setValue("version", 1);
            prefs.sync();
        }
        m_volume = std::clamp(prefs.value("volume", 45).toInt(), 0, 100);
        const auto mode = prefs.value("playMode", "all").toString();
        if (QStringList{"all", "one", "shuffle"}.contains(mode))
            m_playMode = mode;
    }
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(350);
    connect(&m_saveTimer, &QTimer::timeout, this, &MediaState::savePreferences);
}
MediaState::~MediaState() { savePreferences(); }
void MediaState::schedulePreferences() {
    if (m_preferencesEnabled && !m_demoActive)
        m_saveTimer.start();
}
void MediaState::savePreferences() {
    if (!m_preferencesEnabled || m_demoActive)
        return;
    QDir().mkpath(QFileInfo(m_settingsPath).absolutePath());
    QSettings prefs(m_settingsPath, QSettings::IniFormat);
    prefs.setValue("version", 1);
    prefs.setValue("volume", m_volume);
    prefs.setValue("playMode", m_playMode);
    prefs.sync();
}
void MediaState::setDemoActive(bool value) {
    if (value == m_demoActive)
        return;
    if (value) {
        savePreferences();
        m_saveTimer.stop();
    }
    m_demoActive = value;
    if (!value)
        schedulePreferences();
}
QVariantMap MediaState::snapshot() const {
    return {{"volume", m_volume},
            {"track", m_track},
            {"playing", m_playing},
            {"position", m_position},
            {"playMode", m_playMode},
            {"queue", m_queueUrls},
            {"queueExplicit", m_explicitQueue},
            {"queueLabel", m_queueLabel},
            {"musicHistory", m_musicHistory},
            {"historyCursor", m_historyCursor},
            {"shuffleVisited", m_shuffleVisited.values()}};
}
void MediaState::restoreSnapshot(const QVariantMap &s) {
    if (s.isEmpty())
        return;
    m_volume = std::clamp(s["volume"].toInt(), 0, 100);
    m_track = std::clamp(s["track"].toInt(), 0, int(m_tracks.size()) - 1);
    m_playing = s["playing"].toBool();
    m_position = s["position"].toDouble();
    m_playMode = s["playMode"].toString();
    m_queueUrls = s["queue"].toStringList();
    m_explicitQueue = s["queueExplicit"].toBool();
    m_queueLabel = s["queueLabel"].toString();
    m_musicHistory = s["musicHistory"].toStringList();
    m_historyCursor = s["historyCursor"].toInt();
    const auto visited = s["shuffleVisited"].toStringList();
    m_shuffleVisited = QSet<QString>(visited.begin(), visited.end());
    emit changed();
    emit positionChanged();
    emit queueChanged();
    emit seekRequested(qint64(m_position));
    schedulePreferences();
}
void MediaState::resetRuntime() {
    m_playing = false;
    m_position = m_duration = 0;
    m_track = 0;
    m_explicitQueue = false;
    m_queueUrls.clear();
    m_queueLabel = QStringLiteral("全部歌曲");
    resetMusicHistory();
    emit changed();
    emit positionChanged();
    emit queueChanged();
    emit seekRequested(0);
}
void MediaState::setVolume(int value) {
    value = std::clamp(value, 0, 100);
    if (value == m_volume)
        return;
    m_volume = value;
    emit changed();
    schedulePreferences();
}
void MediaState::setPlaying(bool value) {
    if (value && !musicAvailable(m_track))
        value = false;
    if (value == m_playing)
        return;
    m_playing = value;
    emit changed();
}
QString MediaState::trackTitle() const { return m_tracks.value(m_track).toMap().value("title").toString(); }
QString MediaState::trackUrl() const { return m_tracks.value(m_track).toMap().value("url").toString(); }
void MediaState::importTracks(const QVariantList &urls) {
    int count = 0;
    for (const auto &entry : urls) {
        const QUrl url(entry.toUrl());
        const QFileInfo file(url.toLocalFile());
        if (!url.isLocalFile() || !file.isFile())
            continue;
        bool duplicate = false;
        for (const auto &track : m_tracks)
            if (track.toMap()["url"] == url.toString())
                duplicate = true;
        if (duplicate)
            continue;
        m_tracks.append(QVariantMap{{"title", file.completeBaseName()},
                                    {"url", url.toString()},
                                    {"source", QStringLiteral("本地文件")},
                                    {"folder", file.absolutePath()},
                                    {"available", true}});
        ++count;
    }
    if (count) {
        emit changed();
        emit toast(QStringLiteral("已加入 %1 首本地音源").arg(count));
    } else
        emit toast(QStringLiteral("没有可加入的新音源"), "info");
}
void MediaState::mergeMusicTracks(const QVariantList &rows) {
    for (const auto &entry : rows) {
        const auto row = entry.toMap();
        const auto url = row.value("url").toString();
        if (!QUrl(url).isLocalFile())
            continue;
        bool found = false;
        for (auto &track : m_tracks)
            if (track.toMap().value("url") == url) {
                auto value = track.toMap();
                for (auto it = row.cbegin(); it != row.cend(); ++it)
                    value[it.key()] = it.value();
                track = value;
                found = true;
                break;
            }
        if (!found)
            m_tracks.append(row);
    }
    emit changed();
}
void MediaState::updateTrackProperties(const QString &url, const QVariantMap &properties) {
    for (auto &entry : m_tracks) {
        auto row = entry.toMap();
        if (row.value("url") != url)
            continue;
        bool modified = false;
        for (auto it = properties.cbegin(); it != properties.cend(); ++it)
            if (row.value(it.key()) != it.value()) {
                row[it.key()] = it.value();
                modified = true;
            }
        if (modified) {
            entry = row;
            emit changed();
        }
        return;
    }
}
void MediaState::restoreMusicPosition(const QString &url, qint64 position) {
    for (int i = 0; i < m_tracks.size(); ++i)
        if (m_tracks[i].toMap().value("url") == url) {
            m_track = i;
            m_playing = false;
            m_position = qMax(0LL, position);
            m_duration = m_tracks[i].toMap().value("duration", 0).toDouble();
            resetMusicHistory();
            emit changed();
            emit positionChanged();
            return;
        }
}
void MediaState::removeMusicTrack(int index) {
    if (index < 3 || index >= m_tracks.size())
        return;
    const bool current = index == m_track;
    m_tracks.removeAt(index);
    if (index < m_track)
        --m_track;
    else if (current) {
        m_track = qMin(m_track, int(m_tracks.size()) - 1);
        m_playing = false;
        m_position = m_duration = 0;
    }
    emit changed();
    if (current) {
        emit positionChanged();
        emit seekRequested(0);
    }
}
void MediaState::retryTrack() {
    m_mediaError.clear();
    m_mediaStatus = "loading";
    m_playing = true;
    emit mediaReloadRequested();
    emit changed();
}
void MediaState::updateMediaStatus(const QString &value, const QString &error) {
    if (m_mediaStatus == value && m_mediaError == error)
        return;
    m_mediaStatus = value;
    m_mediaError = error;
    if (value == "error")
        m_playing = false;
    emit changed();
}
void MediaState::updatePosition(double value) {
    m_position = std::max(0.0, value);
    emit positionChanged();
}
void MediaState::updateDuration(double value) {
    m_duration = std::max(0.0, value);
    emit positionChanged();
}
void MediaState::seek(double value) {
    if (std::isfinite(value))
        emit seekRequested(qint64(std::clamp(value, 0.0, m_duration)));
}
