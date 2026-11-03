#include "MusicSource.h"
#include "Lyrics.h"
#include <QGuiApplication>
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QMediaMetaData>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QThread>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>

int main(int argc,char **argv){
    qputenv("QT_QPA_PLATFORM","offscreen");qputenv("QT_FORCE_STDERR_LOGGING","1");QGuiApplication app(argc,argv);
    const auto args=app.arguments();if(args.size()!=3){qCritical()<<"Usage: MusicMediaProbe <music-root> <report.json>";return 2;}
    const QDir root(args[1]);if(!root.exists())return 2;QStringList files;
    QDirIterator iterator(root.absolutePath(),QDir::Files|QDir::NoSymLinks,QDirIterator::Subdirectories);
    while(iterator.hasNext()){const auto path=iterator.next();if(QStringList{"mp3","m4a","aac","wav","flac","ogg","opus","wma"}.contains(QFileInfo(path).suffix().toLower()))files<<path;}
    files.sort();if(files.isEmpty()||files.size()>200)return 2;
    QAudioOutput output;output.setMuted(true);std::unique_ptr<QIODevice> device;QMediaPlayer player;player.setAudioOutput(&output);
    auto wait=[&](const std::function<bool()> &ready,int timeout){QElapsedTimer timer;timer.start();while(!ready()&&timer.elapsed()<timeout){app.processEvents();QThread::msleep(10);}return ready();};
    QJsonArray tracks;bool allPassed=true;
    for(const auto &path:files){
        const auto url=QUrl::fromLocalFile(path);setMusicSource(player,url,device);
        wait([&]{return player.duration()>0||player.error()!=QMediaPlayer::NoError;},10000);
        const auto duration=player.duration();bool progressed=false,sought=false;const qint64 target=qMin(60000LL,duration/2);
        if(player.error()==QMediaPlayer::NoError&&duration>1000){
            player.play();progressed=wait([&]{return player.position()>=350||player.error()!=QMediaPlayer::NoError;},6000)&&player.position()>=350;
            player.pause();player.setPosition(target);sought=wait([&]{return qAbs(player.position()-target)<500;},4000);
            if(sought){player.play();sought=wait([&]{return player.position()>target+350||player.error()!=QMediaPlayer::NoError;},6000)&&player.position()>target+350;player.pause();}
        }
        const auto metadata=player.metaData();const auto lyrics=loadLyrics(path);const bool passed=progressed&&sought&&player.error()==QMediaPlayer::NoError;
        tracks.append(QJsonObject{{"file",root.relativeFilePath(path)},{"durationMs",duration},{"title",metadata.stringValue(QMediaMetaData::Title)},{"artist",metadata.stringValue(QMediaMetaData::ContributingArtist)},{"album",metadata.stringValue(QMediaMetaData::AlbumTitle)},
            {"cover",!qvariant_cast<QImage>(metadata.value(QMediaMetaData::CoverArtImage)).isNull()},{"payloadOffset",musicPayloadOffset(path)},{"playbackProgressed",progressed},{"seekPassed",sought},{"seekTargetMs",target},{"timedLyricLines",int(lyrics.lines.size())},{"lyricIndexAtSeek",lyricIndexAt(lyrics.lines,target)},{"error",player.errorString()},{"passed",passed}});
        allPassed&=passed;player.stop();
    }
    player.setSource(QUrl());device.reset();QDir().mkpath(QFileInfo(args[2]).absolutePath());QFile report(args[2]);if(!report.open(QIODevice::WriteOnly))return 2;
    report.write(QJsonDocument(QJsonObject{{"root",root.absolutePath()},{"audioMuted",true},{"allPassed",allPassed},{"tracks",tracks}}).toJson());
    qInfo()<<"Checked"<<tracks.size()<<"actual media files, passed:"<<allPassed;return allPassed?0:1;
}
