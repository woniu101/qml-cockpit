#pragma once
#include <QVariantMap>
#include <QStringList>
QString musicPathKey(const QString &path);
bool supportedMusicFile(const QString &path);
QString musicFileStamp(const QString &path);
QString sidecarCover(const QString &path);
QVariantMap readMusicFile(const QString &path,const QString &cacheDirectory);
