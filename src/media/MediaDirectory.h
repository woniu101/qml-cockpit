#pragma once
#include <QStringList>
#include <QSet>
#include <functional>

QString mediaPathKey(const QString &);
bool mediaPathInside(const QString &path, const QString &root);
struct MediaDirectoryResult {
    QStringList directories;
    int files = 0;
    bool cancelled = false;
};
MediaDirectoryResult visitMediaFiles(const QStringList &paths, const QSet<QString> &extensions,
                                     const std::function<void(const QString &)> &visitor,
                                     const std::function<bool()> &cancelled, int limit = 50000);
