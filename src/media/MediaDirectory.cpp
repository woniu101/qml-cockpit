#include "MediaDirectory.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

QString mediaPathKey(const QString &path) {
    QFileInfo file(path);
    auto key = QDir::cleanPath(file.canonicalFilePath().isEmpty() ? file.absoluteFilePath()
                                                                  : file.canonicalFilePath());
#ifdef Q_OS_WIN
    key = key.toCaseFolded();
#endif
    return key;
}
bool mediaPathInside(const QString &path, const QString &root) {
    const auto key = mediaPathKey(path), base = mediaPathKey(root);
    return key == base || key.startsWith(base.endsWith('/') ? base : base + '/');
}
MediaDirectoryResult visitMediaFiles(const QStringList &paths, const QSet<QString> &extensions,
                                     const std::function<void(const QString &)> &visitor,
                                     const std::function<bool()> &cancelled, int limit) {
    MediaDirectoryResult result;
    QSet<QString> files, directories;
    auto add = [&](const QFileInfo &file) {
        if (cancelled() || files.size() >= limit || !file.isFile() || file.isSymLink() ||
            !extensions.contains(file.suffix().toLower()))
            return;
        const auto key = mediaPathKey(file.absoluteFilePath());
        if (files.contains(key))
            return;
        files.insert(key);
        ++result.files;
        visitor(file.absoluteFilePath());
    };
    for (const auto &path : paths) {
        if (cancelled() || files.size() >= limit)
            break;
        const QFileInfo file(path);
        if (file.isSymLink())
            continue;
        if (file.isDir()) {
            const auto root = mediaPathKey(path);
            if (directories.contains(root))
                continue;
            directories.insert(root);
            result.directories << file.absoluteFilePath();
            QDirIterator entries(path, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks,
                                 QDirIterator::Subdirectories);
            while (entries.hasNext() && !cancelled() && files.size() < limit) {
                entries.next();
                const auto entry = entries.fileInfo();
                if (entry.isDir()) {
                    const auto key = mediaPathKey(entry.absoluteFilePath());
                    if (!directories.contains(key)) {
                        directories.insert(key);
                        result.directories << entry.absoluteFilePath();
                    }
                } else
                    add(entry);
            }
        } else
            add(file);
    }
    result.cancelled = cancelled() || files.size() >= limit;
    return result;
}
