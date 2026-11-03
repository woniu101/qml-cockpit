#pragma once
#include <QIODevice>
#include <QUrl>
#include <memory>
class QMediaPlayer;
qint64 musicPayloadOffset(const QString &path);
// The owner must outlive the player and be reset only after clearing its source.
void setMusicSource(QMediaPlayer &player,const QUrl &url,std::unique_ptr<QIODevice> &owner);
