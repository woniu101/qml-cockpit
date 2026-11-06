#include "MusicSmoke.h"
#include "MusicLibrary.h"
#include "MusicFixture.h"
#include "media/MediaState.h"
#include <QGuiApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QMediaPlayer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QTemporaryDir>
#include <QtTest>
#include <functional>
#include <stdexcept>
void runMusicSmoke(QGuiApplication &app,QQuickWindow &window,MediaState &vehicle,QMediaPlayer &player,MusicLibrary &library,QStringList &warnings){
    window.setProperty("testMode",true);window.setProperty("clockText","16:28");
    QTimer::singleShot(1000,&app,[&]{
        const auto directory=qEnvironmentVariable("CAR_TERMINAL_MUSIC_OUTPUT","artifacts/music/native");QDir().mkpath(directory);const QString root=QFileInfo(directory+"/fixtures").absoluteFilePath(),audio=root+"/歌手/专辑/测试音源.wav";
        auto require=[](bool value,const char *reason){if(!value)throw std::runtime_error(reason);};
        std::function<QQuickItem*(QQuickItem*,const QString&)> find;find=[&](QQuickItem *p,const QString &name)->QQuickItem*{if(p->objectName()==name&&p->isVisible())return p;for(auto *c:p->childItems())if(auto *hit=find(c,name))return hit;return nullptr;};
        auto item=[&](const char *name){auto *hit=find(window.contentItem(),name);require(hit,name);return hit;};
        auto click=[&](const char *name){auto *hit=item(name);require(hit->isEnabled(),name);QTest::mouseClick(&window,Qt::LeftButton,Qt::NoModifier,hit->mapToScene(QPointF(hit->width()/2,hit->height()/2)).toPoint());QTest::qWait(120);};
        auto wait=[&](std::function<bool()> check,const char *reason){for(int i=0;i<1000&&!check();++i)QTest::qWait(20);require(check(),reason);};
        auto capture=[&](const QString &name){window.setProperty("toastMessage","");window.setProperty("feedbackQueue",QVariantList{});QTest::qWait(250);require(window.grabWindow().save(directory+"/"+name+".png"),"music screenshot");};
        try{
            if(app.arguments().contains("--music-resume-check")){
                wait([&]{return player.duration()>14000&&qAbs(player.position()-6000)<200;},"actual player restores persisted position");
                require(!vehicle.playing()&&player.playbackState()!=QMediaPlayer::PlayingState,"restored player stays paused");click("tab3");capture("07-restored-paused");
                require(warnings.isEmpty(),qPrintable(warnings.join('\n')));QFile report(directory+"/resume.json");require(report.open(QIODevice::WriteOnly),"resume evidence");report.write(QJsonDocument(QJsonObject{{"positionMs",player.position()},{"playing",vehicle.playing()},{"actualPlayerPaused",player.playbackState()!=QMediaPlayer::PlayingState}}).toJson());qInfo()<<"PASS real startup music resume, paused at"<<player.position();app.exit(0);return;
            }
            require(writeMusicFixture(audio),"write original metadata/audio fixture");QFile lrc(QFileInfo(audio).absolutePath()+"/测试音源.lrc");require(lrc.open(QIODevice::WriteOnly),"lyrics fixture");lrc.write(QStringLiteral("[00:00]音乐与旅途\n[00:03]窗外的光慢慢移动\n[00:06]这是本地歌词测试\n[00:10]进度与歌词同步\n[00:14]演示结束").toUtf8());lrc.close();
            click("tab3");
            const auto queueCaption=item("musicQueueButton")->property("text");
            library.importFolder(QUrl::fromLocalFile(root));
            require(library.scanning()&&item("musicQueueButton")->property("text")==queueCaption,
                    "the queue button remains visible with the same caption during scanning");
            wait([&]{return !library.scanning();},"recursive scan");require(vehicle.tracks().size()==1,"nested file imported once");
            click("trackRow0");wait([&]{return player.duration()>14000&&vehicle.trackTitle()=="Local Music Test";},"real decode and tagged title");require(library.current()["artist"]=="Cockpit Studio","actual artist metadata");
            vehicle.setPlaying(false);vehicle.seek(6500);QTest::qWait(200);require(library.lyricIndex()==2,"seek synchronizes timed lyrics");capture("01-library-lyrics");
            click("favoriteCurrent");click("musicCategory");click("musicCategoryChoice1");require(library.category()=="favorites"&&library.rows().size()==1,"favorites filtering via combo UI");capture("02-favorites");click("musicCategory");click("musicCategoryChoice4");library.setGroup(root);require(library.rows().size()==1,"parent directory includes nested descendants");capture("03-folders");library.setCategory("all");library.setQuery("nothing matches");require(library.rows().isEmpty(),"search empty state");capture("04-search-empty");library.setQuery("");
            library.seekLyric(3);QTest::qWait(150);require(player.position()>=9900,"lyric jump seeks actual player");library.setLyricOffset(500);require(library.lyricOffset()==500,"lyric adjustment");
            click("musicPhoneAudio");capture("05-phone-source");click("musicLocalSource");require(!vehicle.playing(),"source navigation does not auto-play");
            window.resize(1120,630);QTest::qWait(200);capture("06-minimum-music");
            QTemporaryDir extra;require(extra.isValid(),"temporary queue fixtures");const auto folder=extra.path()+"/queue",second=folder+"/nested/second.wav",third=folder+"/third.wav";
            require(writeMusicFixture(second)&&writeMusicFixture(third),"queue audio fixtures");library.importFolder(QUrl::fromLocalFile(folder));wait([&]{return !library.scanning()&&vehicle.tracks().size()==3;},"second recursive directory");
            library.toggleFavorite(1);library.toggleFavorite(2);library.setCategory("favorites");library.setSort("added");library.playFromView(0);vehicle.setPlaying(false);
            require(vehicle.playQueue().size()==3,"favorites define playback queue");library.setCategory("all");click("musicQueueButton");require(window.property("popup")=="musicQueue","queue panel opens");
            const auto before=vehicle.playQueue();click("queueDown0");require(vehicle.playQueue()[1]==before[0],"UI reorders queue");click("queueUp1");require(vehicle.playQueue()==before,"UI restores queue order");require(!item("queueRemove0")->isEnabled(),"current track retained in queue");capture("08-playback-queue");
            click("queueRemove2");require(vehicle.playQueue().size()==2&&vehicle.tracks().size()==3,"dequeue preserves library");click("closePopup");
            vehicle.setPlayMode("shuffle");const auto first=vehicle.trackUrl();click("fullMusicNext");const auto next=vehicle.trackUrl();require(next!=first&&vehicle.playQueue().contains(next),"shuffle stays in queue");click("fullMusicPrevious");require(vehicle.trackUrl()==first,"previous replays actual shuffle history");click("fullMusicNext");require(vehicle.trackUrl()==next,"next reuses forward history");vehicle.setPlaying(false);
            click("musicFoldersButton");require(window.property("popup")=="musicFolders","directory manager opens");capture("09-directory-manager");wait([&]{return !library.scanning();},"scan idle before directory removal");
            click("removeMusicRoot1");require(library.roots().size()==1&&vehicle.tracks().size()==1,"UI removes exclusively owned tracks");require(QFileInfo::exists(second)&&QFileInfo::exists(third),"directory removal keeps original files");require(!vehicle.playing(),"removing current root pauses actual player");capture("10-directory-removed");click("closePopup");
            require(warnings.isEmpty(),qPrintable(warnings.join('\n')));QFile report(directory+"/verification.json");require(report.open(QIODevice::WriteOnly),"music report");
            report.write(QJsonDocument(QJsonObject{{"recursiveImport",true},{"actualDecode",true},{"actualTitleArtist",true},{"lyricsSeek",true},{"favoritesSearchFolders",true},{"minimumWindow",true},{"scopedQueue",true},{"queueReorderAndRemove",true},{"shuffleHistory",true},{"directoryManager",true},{"originalFilesPreserved",true},{"audio","muted original test fixture"},{"warnings",0}}).toJson());
            qInfo()<<"PASS music UI: library, lyrics, scoped queue, queue reorder/remove, shuffle history, directory removal and minimum window";app.exit(0);
        }catch(const std::exception &error){window.grabWindow().save(directory+"/failure.png");qCritical()<<"MUSIC UI FAIL:"<<error.what();app.exit(2);}
    });
}
