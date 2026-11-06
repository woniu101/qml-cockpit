#include "MediaState.h"
#include <QFileInfo>
#include <QUrl>
#include <QRandomGenerator>

int MediaState::musicIndex(const QString &url) const {
    for (int i = 0; i < m_tracks.size(); ++i)
        if (m_tracks[i].toMap().value("url").toString() == url)
            return i;
    return -1;
}
bool MediaState::musicAvailable(int index) const {
    if (index < 0 || index >= m_tracks.size())
        return false;
    const auto row = m_tracks[index].toMap();
    if (!row.value("available", true).toBool())
        return false;
    const QUrl url(row.value("url").toString());
    return !url.isLocalFile() || QFileInfo(url.toLocalFile()).isFile();
}
QStringList MediaState::playQueue() const {
    QStringList urls;
    if (m_explicitQueue) {
        QSet<QString> existing;
        for (const auto &row : m_tracks)
            existing.insert(row.toMap().value("url").toString());
        for (const auto &url : m_queueUrls)
            if (existing.contains(url))
                urls << url;
    } else
        for (const auto &row : m_tracks)
            urls << row.toMap().value("url").toString();
    return urls;
}
void MediaState::resetMusicHistory() {
    m_musicHistory = {trackUrl()};
    m_historyCursor = 0;
    m_shuffleVisited.clear();
    m_shuffleVisited.insert(trackUrl());
}
void MediaState::setPlayQueue(const QStringList &urls, const QString &label) {
    QSet<QString> existing, seen;
    for (const auto &row : m_tracks)
        existing.insert(row.toMap().value("url").toString());
    QStringList checked;
    for (const auto &url : urls)
        if (existing.contains(url) && !seen.contains(url)) {
            checked << url;
            seen.insert(url);
        }
    if (m_explicitQueue && m_queueUrls == checked && m_queueLabel == label)
        return;
    m_explicitQueue = true;
    m_queueUrls = checked;
    m_queueLabel = label.left(250);
    resetMusicHistory();
    emit queueChanged();
}
void MediaState::setPlayMode(const QString &value) {
    if (!QStringList{"all", "one", "shuffle"}.contains(value) || value == m_playMode)
        return;
    m_playMode = value;
    resetMusicHistory();
    emit changed();
    emit queueChanged();
    schedulePreferences();
}
void MediaState::moveQueueTrack(const QString &url, int direction) {
    auto urls = playQueue();
    const int at = urls.indexOf(url), target = at + (direction < 0 ? -1 : 1);
    if (at < 0 || target < 0 || target >= urls.size())
        return;
    urls.move(at, target);
    m_queueUrls = urls;
    m_explicitQueue = true;
    emit queueChanged();
}
void MediaState::removeQueueTrack(const QString &url) {
    if (url == trackUrl())
        return;
    auto urls = playQueue();
    if (!urls.removeOne(url))
        return;
    m_queueUrls = urls;
    m_explicitQueue = true;
    emit queueChanged();
}
void MediaState::replaceMusicTrack(int index, const QVariantMap &row) {
    if (index < 0 || index >= m_tracks.size())
        return;
    const auto previous = m_tracks[index].toMap().value("url").toString(), url = row.value("url").toString();
    if (url.isEmpty())
        return;
    m_tracks[index] = row;
    for (auto &entry : m_queueUrls)
        if (entry == previous)
            entry = url;
    for (auto &entry : m_musicHistory)
        if (entry == previous)
            entry = url;
    if (m_shuffleVisited.remove(previous))
        m_shuffleVisited.insert(url);
    if (index == m_track) {
        m_playing = false;
        m_position = m_duration = 0;
        m_mediaError.clear();
        m_mediaStatus = "loading";
        emit positionChanged();
    }
    emit changed();
    emit queueChanged();
}
void MediaState::activateMusic(int index, bool recordHistory) {
    if (!musicAvailable(index))
        return;
    const auto url = m_tracks[index].toMap().value("url").toString();
    if (recordHistory) {
        if (m_musicHistory.isEmpty())
            resetMusicHistory();
        if (m_historyCursor < 0 || m_musicHistory.value(m_historyCursor) != url) {
            while (m_musicHistory.size() > m_historyCursor + 1)
                m_musicHistory.removeLast();
            m_musicHistory << url;
            m_historyCursor = m_musicHistory.size() - 1;
            if (m_musicHistory.size() > 500) {
                m_musicHistory.removeFirst();
                --m_historyCursor;
            }
        }
    }
    m_shuffleVisited.insert(url);
    const bool same = index == m_track;
    m_track = index;
    m_position = 0;
    m_playing = true;
    m_mediaError.clear();
    if (same)
        emit seekRequested(0);
    else {
        m_duration = 0;
        m_mediaStatus = "loading";
    }
    emit changed();
    emit positionChanged();
    emit queueChanged();
}
void MediaState::selectTrack(int index) {
    if (!musicAvailable(index)) {
        emit toast(QStringLiteral("文件不可用，请在音乐库重新定位"), "warning");
        return;
    }
    if (index == m_track) {
        setPlaying(!m_playing);
        return;
    }
    const auto url = m_tracks[index].toMap().value("url").toString();
    if (m_explicitQueue && !m_queueUrls.contains(url)) {
        m_explicitQueue = false;
        m_queueUrls.clear();
        m_queueLabel = QStringLiteral("全部歌曲");
        resetMusicHistory();
    }
    activateMusic(index, true);
}
void MediaState::nextTrack(int direction) {
    auto urls = playQueue();
    QHash<QString, int> indexes;
    for (int i = 0; i < m_tracks.size(); ++i)
        indexes.insert(m_tracks[i].toMap().value("url").toString(), i);
    QStringList available;
    for (const auto &url : urls)
        if (musicAvailable(indexes.value(url, -1)))
            available << url;
    if (available.isEmpty()) {
        setPlaying(false);
        emit toast(QStringLiteral("当前播放队列没有可用音源"), "warning");
        return;
    }
    if (m_musicHistory.isEmpty())
        resetMusicHistory();
    if (m_playMode == "shuffle") {
        if (direction < 0) {
            for (int cursor = m_historyCursor - 1; cursor >= 0; --cursor)
                if (available.contains(m_musicHistory[cursor])) {
                    m_historyCursor = cursor;
                    activateMusic(indexes.value(m_musicHistory[cursor]), false);
                    return;
                }
            emit toast(QStringLiteral("已经是本次播放历史的第一首"), "info");
            return;
        }
        for (int cursor = m_historyCursor + 1; cursor < m_musicHistory.size(); ++cursor)
            if (available.contains(m_musicHistory[cursor])) {
                m_historyCursor = cursor;
                activateMusic(indexes.value(m_musicHistory[cursor]), false);
                return;
            }
        QStringList choices;
        for (const auto &url : available)
            if (!m_shuffleVisited.contains(url))
                choices << url;
        if (choices.isEmpty()) {
            m_shuffleVisited.clear();
            m_shuffleVisited.insert(trackUrl());
            for (const auto &url : available)
                if (url != trackUrl())
                    choices << url;
        }
        const auto chosen = choices.isEmpty()
                                ? available.first()
                                : choices[QRandomGenerator::global()->bounded(int(choices.size()))];
        activateMusic(indexes.value(chosen), true);
        return;
    }
    int at = urls.indexOf(trackUrl());
    if (at < 0)
        at = direction < 0 ? 0 : -1;
    for (int i = 0; i < urls.size(); ++i) {
        at = (at + (direction < 0 ? -1 : 1) + urls.size()) % urls.size();
        const auto index = indexes.value(urls[at], -1);
        if (musicAvailable(index)) {
            activateMusic(index, true);
            return;
        }
    }
}
void MediaState::finishTrack() {
    if (m_playMode == "one" && musicAvailable(m_track))
        activateMusic(m_track, false);
    else
        nextTrack();
}
