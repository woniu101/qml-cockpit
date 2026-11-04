#include "VideoLibrary.h"
#include "MediaSession.h"
#include "MediaDirectory.h"
#include <QFile>
#include <QSaveFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMediaPlayer>
#include <QMediaMetaData>
#include <QVideoSink>
#include <QVideoFrame>
#include <QImage>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QBuffer>
#include <QRegularExpression>
#include <algorithm>

namespace {
const QSet<QString> extensions{"mp4", "mkv", "webm", "mov", "avi", "m4v", "mpeg", "mpg", "wmv"};
QString stamp(const QString &path) {
    const QFileInfo file(path);
    return QString::number(file.size()) + ":" + QString::number(file.lastModified().toMSecsSinceEpoch());
}
} // namespace
bool VideoLibrary::supported(const QString &path) {
    return extensions.contains(QFileInfo(path).suffix().toLower());
}
VideoLibrary::VideoLibrary(MediaSession *session, QObject *parent, const QString &storage)
    : QAbstractListModel(parent), m_session(session) {
    m_persist = !storage.isEmpty() || !qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS");
    m_storage = storage.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
                                        "/media/videos.json"
                                  : storage;
    m_cache = m_persist
                  ? QFileInfo(m_storage).absolutePath() + "/video-covers"
                  : qEnvironmentVariable("CAR_TERMINAL_MEDIA_OUTPUT", "artifacts/media") + "/video-covers";
    load();
    refresh();
    connect(session, &MediaSession::videoCheckpoint, this, &VideoLibrary::checkpoint);
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(1200);
    connect(&m_saveTimer, &QTimer::timeout, this, &VideoLibrary::save);
    m_poll.setInterval(5000);
    connect(&m_poll, &QTimer::timeout, this, [this] {
        if (!m_autoSuspended)
            scan(true);
    });
    m_poll.start();
}
VideoLibrary::~VideoLibrary() {
    m_poll.stop();
    if (m_worker) {
        m_worker->requestInterruption();
        m_worker->wait();
    }
    save();
}
QHash<int, QByteArray> VideoLibrary::roleNames() const {
    return {{Url, "videoUrl"},
            {Title, "videoTitle"},
            {Folder, "videoFolder"},
            {Cover, "videoCover"},
            {Duration, "videoDuration"},
            {Bookmark, "videoBookmark"},
            {Available, "videoAvailable"},
            {LastWatched, "videoLastWatched"}};
}
QVariant VideoLibrary::data(const QModelIndex &at, int role) const {
    if (!at.isValid() || at.row() < 0 || at.row() >= m_rows.size())
        return {};
    const auto &entry = m_entries[m_rows[at.row()]];
    switch (role) {
    case Url:
        return entry.url;
    case Title:
        return entry.title;
    case Folder:
        return entry.folder;
    case Cover:
        return entry.cover;
    case Duration:
        return entry.duration;
    case Bookmark:
        return entry.bookmark;
    case Available:
        return entry.available;
    case LastWatched:
        return entry.watched;
    default:
        return {};
    }
}
QStringList VideoLibrary::folders() const {
    QSet<QString> values;
    for (const auto &entry : m_entries)
        values.insert(entry.folder);
    auto result = values.values();
    result.sort();
    return result;
}
void VideoLibrary::refresh() {
    beginResetModel();
    m_rows.clear();
    for (int i = 0; i < m_entries.size(); ++i) {
        const auto &e = m_entries[i];
        if (m_category == "recent" && !e.watched)
            continue;
        if (m_category == "folder" && !m_folder.isEmpty() && !mediaPathInside(e.folder, m_folder))
            continue;
        if (!(e.title + e.folder).contains(m_query.trimmed(), Qt::CaseInsensitive))
            continue;
        m_rows << i;
    }
    std::stable_sort(m_rows.begin(), m_rows.end(), [this](int a, int b) {
        return m_category == "recent"
                   ? m_entries[a].watched > m_entries[b].watched
                   : QString::localeAwareCompare(m_entries[a].title, m_entries[b].title) < 0;
    });
    endResetModel();
    emit changed();
}
void VideoLibrary::setQuery(const QString &value) {
    if (m_query == value)
        return;
    m_query = value.left(200);
    refresh();
}
void VideoLibrary::setCategory(const QString &value) {
    if (!QStringList{"all", "recent", "folder"}.contains(value) || m_category == value)
        return;
    m_category = value;
    refresh();
}
void VideoLibrary::setFolder(const QString &value) {
    if (m_folder == value)
        return;
    m_folder = value;
    refresh();
}
void VideoLibrary::importFiles(const QVariantList &urls) {
    if (scanning()) {
        emit notice(QStringLiteral("请等待视频扫描完成，或先停止扫描"));
        return;
    }
    bool added = false;
    for (const auto &value : urls) {
        const QUrl url = value.toUrl();
        if (!url.isLocalFile())
            continue;
        const QFileInfo info(url.toLocalFile());
        if (info.isSymLink())
            continue;
        const auto path = info.canonicalFilePath();
        if (info.isDir()) {
            bool exists = false;
            for (const auto &root : m_roots)
                exists |= mediaPathKey(root) == mediaPathKey(path);
            if (!exists)
                m_roots << path;
            added = true;
        } else if (info.isFile() && supported(path)) {
            m_manual.insert(path);
            m_excluded.remove(mediaPathKey(path));
            added = true;
        }
    }
    if (added) {
        m_autoSuspended = false;
        scan(false);
    } else
        emit notice(QStringLiteral("请选择有效的视频文件或目录"));
}
void VideoLibrary::importFolder(const QUrl &url) { importFiles({url}); }
VideoLibrary::Entry VideoLibrary::inspect(const QString &path, const QString &cache, qint64 &cacheBytesLeft) {
    const QFileInfo file(path);
    Entry entry;
    entry.url = QUrl::fromLocalFile(file.absoluteFilePath()).toString();
    entry.title = file.completeBaseName();
    entry.folder = file.absolutePath();
    entry.stamp = stamp(path);
    const QString imagePath =
        cache + "/" +
        QCryptographicHash::hash((entry.url + entry.stamp).toUtf8(), QCryptographicHash::Sha256).toHex() +
        ".jpg";
    QMediaPlayer player;
    QVideoSink sink;
    player.setVideoSink(&sink);
    QImage image;
    QObject::connect(&sink, &QVideoSink::videoFrameChanged, &sink, [&](const QVideoFrame &frame) {
        if (image.isNull() && frame.isValid() && qint64(frame.width()) * frame.height() <= 40000000)
            image = frame.toImage();
    });
    player.setSource(QUrl(entry.url));
    player.play();
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 5000 && !QThread::currentThread()->isInterruptionRequested() &&
           player.error() == QMediaPlayer::NoError) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        if (!image.isNull() && player.duration() > 0)
            break;
        QThread::msleep(10);
    }
    entry.duration = player.duration();
    const auto title = player.metaData().stringValue(QMediaMetaData::Title).trimmed();
    if (!title.isEmpty())
        entry.title = title.left(500);
    player.stop();
    player.setSource(QUrl());
    player.setVideoSink(nullptr);
    QObject::disconnect(&sink, nullptr, &sink, nullptr);
    if (!image.isNull() && !QThread::currentThread()->isInterruptionRequested()) {
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        if (image.scaled(560, 315, Qt::KeepAspectRatio, Qt::SmoothTransformation).save(&buffer, "JPG", 85) &&
            bytes.size() <= cacheBytesLeft) {
            QDir().mkpath(cache);
            QSaveFile output(imagePath);
            if (output.open(QIODevice::WriteOnly) && output.write(bytes) == bytes.size() && output.commit()) {
                cacheBytesLeft -= bytes.size();
                entry.cover = QUrl::fromLocalFile(imagePath).toString();
            }
        }
    }
    return entry;
}
void VideoLibrary::scan(bool automatic) {
    if (scanning()) {
        if (!automatic)
            m_pending = true;
        return;
    }
    if (m_roots.isEmpty() && m_manual.isEmpty() && m_entries.isEmpty())
        return;
    QStringList paths = m_roots;
    for (const auto &path : m_manual)
        paths << path;
    QHash<QString, Entry> cached;
    for (const auto &entry : m_entries)
        cached.insert(mediaPathKey(QUrl(entry.url).toLocalFile()), entry);
    struct Result {
        QVector<Entry> entries;
        bool cancelled = false;
    };
    auto result = std::make_shared<Result>();
    const auto excluded = m_excluded;
    const auto cache = m_cache;
    m_scanText = automatic ? QString() : QStringLiteral("正在扫描视频和生成预览…");
    auto *worker = QThread::create([paths, cached, excluded, cache, result] {
        // Only our generated SHA-256 thumbnails are managed here. Originals are never cache files.
        QSet<QString> retained;
        for (const auto &entry : cached)
            retained.insert(mediaPathKey(QUrl(entry.cover).toLocalFile()));
        qint64 cacheBytesLeft = 128LL * 1024 * 1024;
        const QRegularExpression generated("^[a-f0-9]{64}\\.jpg$");
        for (const auto &file : QDir(cache).entryInfoList({"*.jpg"}, QDir::Files | QDir::NoSymLinks)) {
            if (!generated.match(file.fileName()).hasMatch())
                continue;
            if (retained.contains(mediaPathKey(file.absoluteFilePath())) ||
                !QFile::remove(file.absoluteFilePath()))
                cacheBytesLeft -= file.size();
        }
        cacheBytesLeft = qMax(0LL, cacheBytesLeft);
        const auto scan = visitMediaFiles(
            paths, extensions,
            [&](const QString &path) {
                const auto key = mediaPathKey(path);
                if (excluded.contains(key))
                    return;
                auto entry = cached.value(key);
                if (entry.url.isEmpty() || entry.stamp != stamp(path)) {
                    const auto old = entry;
                    entry = inspect(path, cache, cacheBytesLeft);
                    entry.bookmark = old.bookmark;
                    entry.watched = old.watched;
                }
                entry.available = true;
                result->entries << entry;
            },
            [] { return QThread::currentThread()->isInterruptionRequested(); });
        result->cancelled = scan.cancelled;
    });
    m_worker = worker;
    worker->setParent(this);
    connect(worker, &QThread::finished, this, [this, worker, result, automatic] {
        m_worker = nullptr;
        QHash<QString, int> known;
        for (int i = 0; i < m_entries.size(); ++i)
            known.insert(mediaPathKey(QUrl(m_entries[i].url).toLocalFile()), i);
        bool modified = false;
        for (auto entry : result->entries) {
            const auto key = mediaPathKey(QUrl(entry.url).toLocalFile());
            if (known.contains(key)) {
                auto &old = m_entries[known[key]];
                entry.url = old.url;
                entry.bookmark = old.bookmark;
                entry.watched = old.watched;
                modified |= old.stamp != entry.stamp || old.available != entry.available;
                old = entry;
            } else {
                known.insert(key, m_entries.size());
                m_entries << entry;
                modified = true;
            }
        }
        for (auto &entry : m_entries) {
            const bool available = QFileInfo(QUrl(entry.url).toLocalFile()).isFile();
            if (entry.available != available) {
                entry.available = available;
                modified = true;
                if (!available && m_session->itemId() == entry.url)
                    m_session->pause();
            }
        }
        if (!automatic || modified)
            refresh();
        else
            emit changed();
        m_scanText = result->cancelled ? QStringLiteral("扫描已停止，已发现的视频已保留")
                                       : QStringLiteral("视频库共 %1 个文件").arg(m_entries.size());
        worker->deleteLater();
        if (modified || !automatic)
            save();
        emit changed();
        if (m_pending) {
            m_pending = false;
            QTimer::singleShot(0, this, [this] { scan(false); });
        }
    });
    worker->start();
    emit changed();
}
void VideoLibrary::rescan() {
    m_autoSuspended = false;
    scan(false);
}
void VideoLibrary::cancelScan() {
    m_autoSuspended = true;
    m_pending = false;
    if (m_worker) {
        m_worker->requestInterruption();
        m_scanText = QStringLiteral("正在停止扫描…");
        emit changed();
    }
}
QVariantList VideoLibrary::playbackItems() const {
    QVariantList items;
    for (const auto &entry : m_entries)
        items.append(QVariantMap{{"url", entry.url}, {"title", entry.title}, {"available", entry.available}});
    return items;
}

void VideoLibrary::play(const QString &url) {
    for (const auto &entry : m_entries)
        if (entry.url == url) {
            m_session->playVideo(QUrl(url), entry.title, entry.bookmark);
            return;
        }
}
void VideoLibrary::next(int direction) {
    if (m_rows.isEmpty())
        return;
    int at = -1;
    for (int i = 0; i < m_rows.size(); ++i)
        if (m_entries[m_rows[i]].url == m_session->itemId())
            at = i;
    if (at < 0)
        at = direction < 0 ? 0 : m_rows.size() - 1;
    for (int n = 0; n < m_rows.size(); ++n) {
        at = (at + (direction < 0 ? -1 : 1) + m_rows.size()) % m_rows.size();
        const auto &entry = m_entries[m_rows[at]];
        if (entry.available) {
            play(entry.url);
            return;
        }
    }
}
void VideoLibrary::checkpoint(const QUrl &url, qint64 position, qint64 duration) {
    for (auto &entry : m_entries)
        if (entry.url == url.toString()) {
            const auto now = QDateTime::currentMSecsSinceEpoch();
            const auto bookmark = duration - position < 3000 ? 0 : position;
            const bool changed = entry.bookmark != bookmark || !entry.watched;
            const bool updateOrder = !entry.watched || now - entry.watched > 5000;
            entry.bookmark = bookmark;
            entry.duration = duration;
            if (updateOrder)
                entry.watched = now;
            if (changed || updateOrder) {
                if (m_category == "recent" && updateOrder)
                    refresh();
                else if (!m_rows.isEmpty())
                    emit dataChanged(index(0), index(m_rows.size() - 1));
                if (!m_saveTimer.isActive())
                    m_saveTimer.start();
            }
            return;
        }
}
void VideoLibrary::remove(const QString &url) {
    if (scanning()) {
        emit notice(QStringLiteral("停止扫描后再移除视频"));
        return;
    }
    for (int i = 0; i < m_entries.size(); ++i)
        if (m_entries[i].url == url) {
            m_excluded.insert(mediaPathKey(QUrl(url).toLocalFile()));
            m_manual.remove(QUrl(url).toLocalFile());
            if (m_session->itemId() == url)
                m_session->stop();
            m_entries.removeAt(i);
            refresh();
            save();
            return;
        }
}
void VideoLibrary::removeRoot(const QString &root) {
    if (scanning()) {
        emit notice(QStringLiteral("停止扫描后再移除目录"));
        return;
    }
    m_roots.removeAll(root);
    for (int i = m_entries.size() - 1; i >= 0; --i) {
        const auto path = QUrl(m_entries[i].url).toLocalFile();
        if (!mediaPathInside(path, root) || m_manual.contains(path))
            continue;
        bool shared = false;
        for (const auto &remaining : m_roots)
            shared |= mediaPathInside(path, remaining);
        if (!shared) {
            if (m_session->itemId() == m_entries[i].url)
                m_session->stop();
            m_entries.removeAt(i);
        }
    }
    m_folder.clear();
    refresh();
    save();
}
void VideoLibrary::relocate(const QString &previous, const QUrl &url) {
    if (scanning() || !url.isLocalFile() || !QFileInfo(url.toLocalFile()).isFile() ||
        !supported(url.toLocalFile()))
        return;
    for (auto &entry : m_entries)
        if (entry.url == previous) {
            for (const auto &other : m_entries)
                if (other.url != previous &&
                    mediaPathKey(QUrl(other.url).toLocalFile()) == mediaPathKey(url.toLocalFile())) {
                    emit notice(QStringLiteral("该视频已经在库中"));
                    return;
                }
            if (m_session->itemId() == previous)
                m_session->stop();
            m_excluded.insert(mediaPathKey(QUrl(previous).toLocalFile()));
            m_manual.remove(QUrl(previous).toLocalFile());
            const auto bookmark = entry.bookmark, watched = entry.watched;
            entry = {};
            entry.url = url.toString();
            entry.title = QFileInfo(url.toLocalFile()).completeBaseName();
            entry.folder = QFileInfo(url.toLocalFile()).absolutePath();
            entry.bookmark = bookmark;
            entry.watched = watched;
            m_manual.insert(url.toLocalFile());
            m_excluded.remove(mediaPathKey(url.toLocalFile()));
            refresh();
            scan(false);
            return;
        }
}
void VideoLibrary::save() {
    if (!m_persist)
        return;
    QJsonArray entries;
    for (const auto &e : m_entries)
        entries << QJsonObject{{"url", e.url},           {"title", e.title},    {"folder", e.folder},
                               {"cover", e.cover},       {"stamp", e.stamp},    {"duration", e.duration},
                               {"bookmark", e.bookmark}, {"watched", e.watched}};
    QDir().mkpath(QFileInfo(m_storage).absolutePath());
    QSaveFile file(m_storage);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(QJsonObject{{"version", 1},
                                             {"entries", entries},
                                             {"roots", QJsonArray::fromStringList(m_roots)},
                                             {"manual", QJsonArray::fromStringList(m_manual.values())},
                                             {"excluded", QJsonArray::fromStringList(m_excluded.values())}})
                       .toJson());
        file.commit();
    }
}
void VideoLibrary::load() {
    if (!m_persist)
        return;
    QFile file(m_storage);
    if (!file.exists())
        return;
    if (file.size() > 32 * 1024 * 1024 || !file.open(QIODevice::ReadOnly)) {
        m_persist = false;
        return;
    }
    QJsonParseError error;
    const auto object = QJsonDocument::fromJson(file.readAll(), &error).object();
    if (error.error != QJsonParseError::NoError || object["version"].toInt() != 1) {
        m_persist = false;
        return;
    }
    for (const auto &value : object["roots"].toArray())
        if (QDir::isAbsolutePath(value.toString()))
            m_roots << QDir::cleanPath(value.toString());
    for (const auto &value : object["manual"].toArray())
        if (QDir::isAbsolutePath(value.toString()))
            m_manual.insert(value.toString());
    for (const auto &value : object["excluded"].toArray())
        m_excluded.insert(value.toString());
    for (const auto &value : object["entries"].toArray()) {
        const auto e = value.toObject();
        const QUrl url(e["url"].toString());
        if (!url.isLocalFile() || !supported(url.toLocalFile()))
            continue;
        m_entries << Entry{url.toString(),
                           e["title"].toString(),
                           QFileInfo(url.toLocalFile()).absolutePath(),
                           e["cover"].toString(),
                           e["stamp"].toString(),
                           qMax(0LL, e["duration"].toInteger()),
                           qMax(0LL, e["bookmark"].toInteger()),
                           qMax(0LL, e["watched"].toInteger()),
                           QFileInfo(url.toLocalFile()).isFile()};
    }
}
