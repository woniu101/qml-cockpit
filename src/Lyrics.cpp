#include "Lyrics.h"
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringDecoder>
#include <algorithm>
#ifdef Q_OS_WIN
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

QString decodeLyrics(const QByteArray &data){
    if(data.startsWith("\xff\xfe")||data.startsWith("\xfe\xff")){QStringDecoder decoder(QStringConverter::Utf16);return decoder(data);}
    QStringDecoder utf8(QStringConverter::Utf8);const QString text=utf8(data);if(!utf8.hasError())return text;
#ifdef Q_OS_WIN
    const int length=MultiByteToWideChar(54936,MB_ERR_INVALID_CHARS,data.data(),int(data.size()),nullptr,0);
    if(length>0){QString result(length,Qt::Uninitialized);MultiByteToWideChar(54936,MB_ERR_INVALID_CHARS,data.data(),int(data.size()),reinterpret_cast<wchar_t*>(result.data()),length);return result;}
#endif
    return text;
}
LyricsDocument parseLyrics(const QString &text){
    LyricsDocument result;QList<QPair<qint64,QString>> cues;
    static const QRegularExpression time(R"(\[(\d{1,4}):(\d{1,2})(?:[.:](\d{1,3}))?\])");
    static const QRegularExpression offset(R"(\[offset\s*:\s*([+-]?\d+)\])",QRegularExpression::CaseInsensitiveOption);
    const auto offsetMatch=offset.match(text);const qint64 shift=offsetMatch.hasMatch()?std::clamp(offsetMatch.captured(1).toLongLong(),-3600000LL,3600000LL):0;
    for(const auto &line:text.split('\n')){
        QList<qint64> times;auto matches=time.globalMatch(line);int end=0;
        while(matches.hasNext()){
            const auto match=matches.next();if(!line.mid(end,match.capturedStart()-end).trimmed().isEmpty())break;
            end=match.capturedEnd();const int seconds=match.captured(2).toInt();if(seconds>=60)continue;
            const auto fraction=match.captured(3);const int millis=fraction.isEmpty()?0:fraction.leftJustified(3,'0').toInt();
            times<<qMax(0LL,match.captured(1).toLongLong()*60000+seconds*1000+millis-shift);
        }
        const auto content=line.mid(end).trimmed();for(auto at:times)cues.append({at,content});
    }
    std::stable_sort(cues.begin(),cues.end(),[](const auto &a,const auto &b){return a.first<b.first;});
    for(const auto &cue:cues){
        if(!result.lines.isEmpty()&&result.lines.last().toMap()["at"].toLongLong()==cue.first){auto row=result.lines.last().toMap();if(!cue.second.isEmpty())row["text"]=row["text"].toString()+(row["text"].toString().isEmpty()?"":"\n")+cue.second;result.lines.last()=row;}
        else result.lines<<QVariantMap{{"at",cue.first},{"text",cue.second}};
    }
    if(result.lines.isEmpty())result.plainText=text.trimmed();return result;
}
LyricsDocument loadLyrics(const QString &audioPath){
    const QFileInfo audio(audioPath);const QString base=audio.absolutePath()+"/"+audio.completeBaseName();
    QString path=base+".lrc";if(!QFileInfo::exists(path))path=base+".txt";
    QFile file(path);if(!file.exists())return {};
    if(file.size()>2*1024*1024)return {{},{},QStringLiteral("歌词文件过大，请使用小于 2 MB 的文本")};
    if(!file.open(QIODevice::ReadOnly))return {{},{},QStringLiteral("歌词文件无法读取")};
    return parseLyrics(decodeLyrics(file.readAll()));
}
int lyricIndexAt(const QVariantList &lines,qint64 position){
    auto next=std::upper_bound(lines.cbegin(),lines.cend(),position,[](qint64 at,const QVariant &row){return at<row.toMap().value("at").toLongLong();});
    return int(next-lines.cbegin())-1;
}
