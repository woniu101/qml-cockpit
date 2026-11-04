#include <QtTest>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QDataStream>
#include <QImage>
#include <QMediaPlayer>
#include <QStringEncoder>
#include "Lyrics.h"
#include "MusicLibrary.h"
#include "MusicFiles.h"
#include "media/MediaState.h"
#include "MusicFixture.h"
#include "MusicSource.h"

static void wave(const QString &path){
    QDir().mkpath(QFileInfo(path).absolutePath());QFile file(path);if(!file.open(QIODevice::WriteOnly))qFatal("Cannot create wave fixture");
    QDataStream stream(&file);stream.setByteOrder(QDataStream::LittleEndian);const QByteArray silence(22050*2*15,'\0');
    stream.writeRawData("RIFF",4);stream<<quint32(36+silence.size());stream.writeRawData("WAVEfmt ",8);stream<<quint32(16)<<quint16(1)<<quint16(1)<<quint32(22050)<<quint32(44100)<<quint16(2)<<quint16(16);stream.writeRawData("data",4);stream<<quint32(silence.size());stream.writeRawData(silence.data(),silence.size());
}
class MusicLibraryTests:public QObject {
    Q_OBJECT
private slots:
    void initTestCase(){qputenv("CAR_TERMINAL_NO_SETTINGS","1");}
    void lyricsTimingEncodingAndDuplicates(){
        auto doc=parseLyrics("[ti:test]\n[offset:500]\n[00:01.5][00:03.250]first\n[00:03.250]translation\n[00:02]middle\n[00:99]invalid");
        QCOMPARE(doc.lines.size(),3);QCOMPARE(doc.lines[0].toMap()["at"].toLongLong(),1000);QCOMPARE(doc.lines[1].toMap()["at"].toLongLong(),1500);QCOMPARE(doc.lines[2].toMap()["text"].toString(),"first\ntranslation");
        QCOMPARE(lyricIndexAt(doc.lines,999),-1);QCOMPARE(lyricIndexAt(doc.lines,1000),0);QCOMPARE(lyricIndexAt(doc.lines,1499),0);QCOMPARE(lyricIndexAt(doc.lines,1500),1);QCOMPARE(lyricIndexAt(doc.lines,999999),2);
        const QString text=QStringLiteral("[00:01]你好");QCOMPARE(decodeLyrics(text.toUtf8()),text);QStringEncoder encoder(QStringConverter::Utf16LE);QCOMPARE(decodeLyrics(QByteArray("\xff\xfe")+encoder(text)),text);
#ifdef Q_OS_WIN
        QCOMPARE(decodeLyrics(QByteArray::fromHex("c4e3bac3")),QStringLiteral("你好"));
#endif
        QVERIFY(parseLyrics("plain lyrics").lines.isEmpty());QCOMPARE(parseLyrics("plain lyrics").plainText,"plain lyrics");
    }
    void recursiveImportFilteringPersistenceAndMissing(){
        QTemporaryDir temp;QVERIFY(temp.isValid());const auto root=temp.path()+"/music",a=root+"/歌手/专辑/one.wav",b=root+"/other/two.WAV",store=temp.path()+"/state/library.json";wave(a);wave(b);
        QFile lyrics(QFileInfo(a).absolutePath()+"/one.lrc");QVERIFY(lyrics.open(QIODevice::WriteOnly));lyrics.write("[00:01]first\n[00:03]second");lyrics.close();
        QImage cover(20,20,QImage::Format_RGB32);cover.fill(Qt::green);QVERIFY(cover.save(QFileInfo(a).absolutePath()+"/cover.png"));
        {
            MediaState state;QMediaPlayer player;MusicLibrary library(&state,&player,nullptr,store);library.importFolder(QUrl::fromLocalFile(root));QTRY_VERIFY_WITH_TIMEOUT(!library.scanning(),30000);QCOMPARE(state.tracks().size(),5);
            library.importFiles({QUrl::fromLocalFile(a),QUrl::fromLocalFile(b)});QTRY_VERIFY_WITH_TIMEOUT(!library.scanning(),30000);QCOMPARE(state.tracks().size(),5);
            int index=-1;for(int i=3;i<state.tracks().size();++i)if(QUrl(state.tracks()[i].toMap()["url"].toString()).toLocalFile()==a)index=i;QVERIFY(index>=3);
            library.toggleFavorite(index);state.selectTrack(index);state.setPlaying(false);state.updateDuration(15000);state.updatePosition(2300);QCOMPARE(library.lyrics().size(),2);QCOMPARE(library.lyricIndex(),0);
            library.setLyricOffset(500);state.updatePosition(3200);QCOMPARE(library.lyricIndex(),0);state.updatePosition(3600);QCOMPARE(library.lyricIndex(),1);
            QSignalSpy seeks(&state,&MediaState::seekRequested);library.seekLyric(0);QCOMPARE(seeks.last()[0].toLongLong(),1500);
            library.setCategory("favorites");QCOMPARE(library.rows().size(),1);library.setQuery("nomatch");QVERIFY(library.rows().isEmpty());library.setQuery("");library.setCategory("folder");library.setGroup(root);QCOMPARE(library.rows().size(),2);
            QVERIFY(!library.current()["cover"].toString().isEmpty());library.save();
        }
        QVERIFY(QFile::remove(a));
        {
            MediaState state;QMediaPlayer player;MusicLibrary library(&state,&player,nullptr,store);QCOMPARE(state.tracks().size(),5);QVERIFY(!state.playing());QCOMPARE(qint64(state.position()),3600);QVERIFY(!library.current()["available"].toBool());QVERIFY(library.current()["favorite"].toBool());QCOMPARE(library.lyricOffset(),500);
            library.remove(state.track());QCOMPARE(state.tracks().size(),4);QVERIFY(!state.playing());wave(a);library.rescan();QTRY_VERIFY_WITH_TIMEOUT(!library.scanning(),30000);QCOMPARE(state.tracks().size(),4);QVERIFY(QFileInfo::exists(a));
        }
    }
    void cancelAndCorruptStorePreserved(){
        QTemporaryDir temp;const auto store=temp.path()+"/bad.json";QFile file(store);QVERIFY(file.open(QIODevice::WriteOnly));file.write("broken cache");file.close();
        MediaState state;QMediaPlayer player;MusicLibrary library(&state,&player,nullptr,store);QVERIFY(library.scanText().contains(QStringLiteral("格式异常")));library.save();QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),QByteArray("broken cache"));
        const auto root=temp.path()+"/music";for(int i=0;i<8;++i)wave(root+"/"+QString::number(i)+".wav");library.importFolder(QUrl::fromLocalFile(root));library.cancelScan();QTRY_VERIFY_WITH_TIMEOUT(!library.scanning(),15000);QVERIFY(library.scanText().contains(QStringLiteral("停止")));
    }
    void actualMetadataAndEmbeddedArtwork(){
        QTemporaryDir temp;const auto path=temp.path()+"/song.wav";QVERIFY(writeMusicFixture(path));MediaState state;QMediaPlayer player;MusicLibrary library(&state,&player,nullptr,temp.path()+"/library.json");
        library.importFiles({QUrl::fromLocalFile(path)});QTRY_VERIFY_WITH_TIMEOUT(!library.scanning(),15000);QCOMPARE(state.tracks().size(),4);state.selectTrack(3);state.setPlaying(false);player.setSource(QUrl::fromLocalFile(path));
        QTRY_VERIFY_WITH_TIMEOUT(player.mediaStatus()==QMediaPlayer::LoadedMedia||player.mediaStatus()==QMediaPlayer::BufferedMedia,10000);library.flushMetadata();
        QCOMPARE(library.current()["title"].toString(),"Local Music Test");QCOMPARE(library.current()["artist"].toString(),"Cockpit Studio");QCOMPARE(library.current()["album"].toString(),"Offline Sessions");
        const QImage cover(QUrl(library.current()["cover"].toString()).toLocalFile());QVERIFY(!cover.isNull());QVERIFY(cover.pixelColor(0,0).green()>cover.pixelColor(0,0).red());QVERIFY(!state.playing());
    }
    void queueScopeShuffleHistoryAndStableUrls(){
        QTemporaryDir temp;MediaState state;QMediaPlayer player;MusicLibrary library(&state,&player);
        QVariantList imports;for(int i=0;i<4;++i){const auto path=temp.path()+"/"+QString::number(i)+".wav";wave(path);imports<<QUrl::fromLocalFile(path);}
        library.importFiles(imports);QTRY_VERIFY_WITH_TIMEOUT(!library.scanning(),15000);QCOMPARE(state.tracks().size(),7);
        for(int index:{3,5,6})library.toggleFavorite(index);library.setCategory("favorites");library.setSort("added");library.playFromView(3);
        const auto queue=state.playQueue();QCOMPARE(queue.size(),3);QCOMPARE(state.trackUrl(),queue[0]);state.nextTrack();QCOMPARE(state.trackUrl(),queue[1]);state.finishTrack();QCOMPARE(state.trackUrl(),queue[2]);
        library.setCategory("all");library.setQuery("does not exist");state.nextTrack();QCOMPARE(state.trackUrl(),queue[0]);QCOMPARE(state.playQueue(),queue);
        library.remove(4);state.nextTrack();QCOMPARE(state.trackUrl(),queue[1]); // indexes shifted, identities did not
        state.setPlayMode("shuffle");QStringList history{state.trackUrl()};state.nextTrack();history<<state.trackUrl();state.nextTrack();history<<state.trackUrl();QCOMPARE(QSet<QString>(history.begin(),history.end()).size(),3);
        state.nextTrack(-1);QCOMPARE(state.trackUrl(),history[1]);state.nextTrack(-1);QCOMPARE(state.trackUrl(),history[0]);state.nextTrack(-1);QCOMPARE(state.trackUrl(),history[0]);state.nextTrack();QCOMPARE(state.trackUrl(),history[1]);state.nextTrack();QCOMPARE(state.trackUrl(),history[2]);
        const auto previous=state.trackUrl();state.nextTrack();QVERIFY(state.trackUrl()!=previous);QVERIFY(queue.contains(state.trackUrl()));
        state.setPlayMode("all");const auto removed=queue[1];QVERIFY(QFile::remove(QUrl(removed).toLocalFile()));for(int i=0;i<5;++i){state.nextTrack();QVERIFY(state.trackUrl()!=removed);}
        state.setPlayQueue({},"empty");state.nextTrack();QVERIFY(!state.playing());QVERIFY(state.playQueue().isEmpty());
    }
    void removeRootsPreservesSharedManualAndFiles(){
        QTemporaryDir temp;const auto root=temp.path()+"/root",sub=root+"/nested",a=root+"/a.wav",b=sub+"/b.wav",c=root+"/c.wav",store=temp.path()+"/library.json";wave(a);wave(b);wave(c);
        {MediaState state;QMediaPlayer player;MusicLibrary library(&state,&player,nullptr,store);
            library.importFolder(QUrl::fromLocalFile(root));QTRY_VERIFY_WITH_TIMEOUT(!library.scanning(),15000);library.importFolder(QUrl::fromLocalFile(sub));QTRY_VERIFY_WITH_TIMEOUT(!library.scanning(),15000);library.importFiles({QUrl::fromLocalFile(c)});QTRY_VERIFY_WITH_TIMEOUT(!library.scanning(),15000);QCOMPARE(state.tracks().size(),6);
            library.removeRoot(root);QCOMPARE(library.roots(),QStringList{sub});QCOMPARE(state.tracks().size(),5);library.removeRoot(sub);QVERIFY(library.roots().isEmpty());QCOMPARE(state.tracks().size(),4);QCOMPARE(QUrl(state.tracks()[3].toMap()["url"].toString()).toLocalFile(),c);QVERIFY(QFileInfo::exists(a)&&QFileInfo::exists(b)&&QFileInfo::exists(c));library.save();
        }
        {MediaState state;QMediaPlayer player;MusicLibrary library(&state,&player,nullptr,store);QVERIFY(library.roots().isEmpty());QCOMPARE(state.tracks().size(),4);}
    }
    void watcherFindsNestedFilesAndHandlesDetachRecovery(){
        QTemporaryDir temp;const auto root=temp.path()+"/disk",audio=root+"/new/nested/music.wav";QVERIFY(QDir().mkpath(root));MediaState state;QMediaPlayer player;MusicLibrary library(&state,&player);
        library.importFolder(QUrl::fromLocalFile(root));QTRY_VERIFY_WITH_TIMEOUT(!library.scanning(),15000);QCOMPARE(state.tracks().size(),3);wave(audio);
        QTRY_COMPARE_WITH_TIMEOUT(state.tracks().size(),4,15000);QTRY_VERIFY_WITH_TIMEOUT(!library.scanning(),15000);library.playFromView(3);QVERIFY(state.playing());const auto url=state.trackUrl();
        QVERIFY(QFile::remove(audio));QTRY_VERIFY_WITH_TIMEOUT(!state.playing(),10000);QTRY_VERIFY_WITH_TIMEOUT(!library.current()["available"].toBool(),10000);QCOMPARE(state.trackUrl(),url);
        wave(audio);QTRY_VERIFY_WITH_TIMEOUT(library.current()["available"].toBool(),15000);QVERIFY(!state.playing());QTRY_VERIFY_WITH_TIMEOUT(!library.scanning(),15000);
        QFile lrc(QFileInfo(audio).absolutePath()+"/music.lrc");QVERIFY(lrc.open(QIODevice::WriteOnly));lrc.write("[00:01]changed lyrics");lrc.close();QTRY_COMPARE_WITH_TIMEOUT(library.lyrics().size(),1,15000);
        QTRY_VERIFY_WITH_TIMEOUT(!library.scanning(),15000);QSignalSpy lyricReload(&library,&MusicLibrary::lyricsChanged);QTest::qWait(5500);QCOMPARE(lyricReload.count(),0);
        QTRY_VERIFY_WITH_TIMEOUT(!library.scanning(),15000);state.setPlaying(true);const bool detached=QDir().rename(root,root+"-detached");QVERIFY(detached);QTRY_VERIFY_WITH_TIMEOUT(!library.current()["available"].toBool(),10000);QVERIFY(!state.playing());QVERIFY(!library.rootEntries()[0].toMap()["available"].toBool());
        QVERIFY(QDir().rename(root+"-detached",root));QTRY_VERIFY_WITH_TIMEOUT(library.current()["available"].toBool(),15000);QVERIFY(!state.playing());QVERIFY(library.rootEntries()[0].toMap()["available"].toBool());
    }
    void scopedQueuePersistsPaused(){
        QTemporaryDir temp;const auto store=temp.path()+"/library.json";QStringList expected;
        {MediaState state;QMediaPlayer player;MusicLibrary library(&state,&player,nullptr,store);QVariantList urls;for(int i=0;i<3;++i){const auto path=temp.path()+"/"+QString::number(i)+".wav";wave(path);urls<<QUrl::fromLocalFile(path);}library.importFiles(urls);QTRY_VERIFY_WITH_TIMEOUT(!library.scanning(),15000);
            library.toggleFavorite(3);library.toggleFavorite(5);library.setCategory("favorites");library.setSort("added");library.playFromView(3);expected=state.playQueue();state.setPlaying(false);library.save();}
        {MediaState state;QMediaPlayer player;MusicLibrary library(&state,&player,nullptr,store);QCOMPARE(state.playQueue(),expected);QVERIFY(!state.playing());state.nextTrack();QCOMPARE(state.trackUrl(),expected[1]);}
    }
    void queueEditsPreserveHistoryAndLibrary(){
        MediaState state;const auto original=state.playQueue();state.setPlayMode("shuffle");state.nextTrack();const auto second=state.trackUrl();state.nextTrack(-1);const auto first=state.trackUrl();
        const int before=state.playQueue().indexOf(second);state.moveQueueTrack(second,-1);QCOMPARE(state.playQueue().indexOf(second),before-1);state.nextTrack();QCOMPARE(state.trackUrl(),second);state.nextTrack(-1);QCOMPARE(state.trackUrl(),first);
        state.removeQueueTrack(first);QCOMPARE(state.playQueue().size(),3);state.removeQueueTrack(second);QCOMPARE(state.playQueue().size(),2);QCOMPARE(state.tracks().size(),3);state.nextTrack();QVERIFY(state.trackUrl()!=second);QVERIFY(original.contains(state.trackUrl()));
    }
    void relocationKeepsQueueAndExcludesOldPath(){
        QTemporaryDir temp;const auto root=temp.path()+"/root",a=root+"/a.wav",b=root+"/b.wav",replacement=temp.path()+"/moved.wav";wave(a);wave(b);wave(replacement);
        MediaState state;QMediaPlayer player;MusicLibrary library(&state,&player);library.importFolder(QUrl::fromLocalFile(root));QTRY_VERIFY_WITH_TIMEOUT(!library.scanning(),15000);
        library.setCategory("folder");library.setGroup(root);library.playFromView(3);const auto before=state.playQueue();QCOMPARE(before.size(),2);const auto expected=QUrl::fromLocalFile(replacement).toString();
        library.relocate(3,QUrl::fromLocalFile(replacement));QTRY_VERIFY_WITH_TIMEOUT(!library.scanning(),15000);QCOMPARE(state.tracks().size(),5);QCOMPARE(state.playQueue()[0],expected);QCOMPARE(state.playQueue()[1],before[1]);QCOMPARE(state.trackUrl(),expected);QVERIFY(!state.playing());
        library.rescan();QTRY_VERIFY_WITH_TIMEOUT(!library.scanning(),15000);QCOMPARE(state.tracks().size(),5);QVERIFY(!state.playQueue().contains(before[0]));QVERIFY(QFileInfo::exists(a));
    }
    void prefixedContainerDetectionIsConservative(){
        QTemporaryDir temp;const auto fileName=temp.path()+"/example.mp3";QFile file(fileName);QVERIFY(file.open(QIODevice::WriteOnly));
        QByteArray header("ID3\x03\x00\x00\x00\x00\x00\x04",10);const auto payload=QByteArray::fromHex("00000018667479704d3441200000000069736f6d6d703432");file.write(header);file.write("tags",4);file.write(payload);file.close();QCOMPARE(musicPayloadOffset(fileName),14);
        QVERIFY(file.open(QIODevice::WriteOnly));file.write(header);file.write("tags",4);file.write(QByteArray(24,'x'));file.close();QCOMPARE(musicPayloadOffset(fileName),0);
        QVERIFY(file.open(QIODevice::WriteOnly));header[6]=char(0x80);file.write(header);file.write("tags",4);file.write(payload);file.close();QCOMPARE(musicPayloadOffset(fileName),0);
        const auto normal=temp.path()+"/regular.wav";wave(normal);QCOMPARE(musicPayloadOffset(normal),0);
    }
    void prefixedId3MetadataAndArtwork(){
        QTemporaryDir temp;const auto wavePath=temp.path()+"/source.wav",path=temp.path()+"/wrapped.mp3";QVERIFY(writeMusicFixture(wavePath));QFile original(wavePath);QVERIFY(original.open(QIODevice::ReadOnly));const auto waveBytes=original.readAll();const auto start=waveBytes.lastIndexOf("id3 ");QVERIFY(start>=0);
        const auto header=waveBytes.mid(start+8,10);quint32 tagSize=0;for(int i=6;i<10;++i)tagSize=(tagSize<<7)|quint8(header[i]);const auto tags=waveBytes.mid(start+8,10+tagSize);QFile wrapped(path);QVERIFY(wrapped.open(QIODevice::WriteOnly));wrapped.write(tags);wrapped.write(QByteArray::fromHex("00000018667479704d3441200000000069736f6d6d703432"));wrapped.close();
        const auto row=readMusicFile(path,temp.path()+"/covers");QCOMPARE(row["title"].toString(),"Local Music Test");QCOMPARE(row["artist"].toString(),"Cockpit Studio");QCOMPARE(row["album"].toString(),"Offline Sessions");QVERIFY(!QImage(QUrl(row["cover"].toString()).toLocalFile()).isNull());
    }
};
QTEST_GUILESS_MAIN(MusicLibraryTests)
#include "MusicLibraryTests.moc"
