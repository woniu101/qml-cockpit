#pragma once
#include <QString>
#include <QVariantList>
struct LyricsDocument { QVariantList lines; QString plainText,error; };
QString decodeLyrics(const QByteArray &data);
LyricsDocument parseLyrics(const QString &text);
LyricsDocument loadLyrics(const QString &audioPath);
int lyricIndexAt(const QVariantList &lines,qint64 position);
