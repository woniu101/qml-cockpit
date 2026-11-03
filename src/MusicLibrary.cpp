#include "MusicLibrary.h"
#include "MusicFiles.h"
#include "VehicleState.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QMediaMetaData>
#include <QCryptographicHash>
#include <QImage>
#include <QScopeGuard>
#include <QDateTime>
#include <algorithm>
#ifdef Q_OS_WIN
#include <winrt/base.h>
#endif

namespace {
bool insideRoot(const QString &path,const QString &root){const auto key=musicPathKey(path),base=musicPathKey(root);return key==base||key.startsWith(base.endsWith('/')?base:base+"/");}
QString lyricsStamp(const QString &url){const QFileInfo file(QUrl(url).toLocalFile());QString path=file.absolutePath()+"/"+file.completeBaseName()+".lrc";if(!QFileInfo::exists(path))path=file.absolutePath()+"/"+file.completeBaseName()+".txt";const QFileInfo lyrics(path);return url+path+QString::number(lyrics.size())+QString::number(lyrics.lastModified().toMSecsSinceEpoch());}
}

MusicLibrary::MusicLibrary(VehicleState *vehicle,QMediaPlayer *player,QObject *parent,const QString &storageFile):QObject(parent),m_vehicle(vehicle),m_player(player){
    m_persist=!storageFile.isEmpty()||!qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS");
    m_storage=storageFile.isEmpty()?QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+"/music/library.json":storageFile;
    m_cache=m_persist?QFileInfo(m_storage).absolutePath()+"/covers":qEnvironmentVariableIsSet("CAR_TERMINAL_MUSIC_OUTPUT")?qEnvironmentVariable("CAR_TERMINAL_MUSIC_OUTPUT")+"/covers":QString();
    m_saveTimer.setSingleShot(true);m_saveTimer.setInterval(1200);connect(&m_saveTimer,&QTimer::timeout,this,&MusicLibrary::save);
    m_checkpoint.setInterval(5000);connect(&m_checkpoint,&QTimer::timeout,this,&MusicLibrary::save);if(m_persist)m_checkpoint.start();
    load();connect(vehicle,&VehicleState::changed,this,&MusicLibrary::stateChanged);connect(vehicle,&VehicleState::positionChanged,this,&MusicLibrary::updateLyricPosition);
    connect(player,&QMediaPlayer::metaDataChanged,this,&MusicLibrary::metadataChanged);stateChanged();
    connect(vehicle,&VehicleState::queueChanged,this,[this]{emit changed();if(!m_loading)m_saveTimer.start();});
    m_changeTimer.setSingleShot(true);m_changeTimer.setInterval(500);connect(&m_changeTimer,&QTimer::timeout,this,&MusicLibrary::automaticRefresh);
    connect(&m_watcher,&QFileSystemWatcher::directoryChanged,this,[this]{requestRefresh();});
    connect(&m_watcher,&QFileSystemWatcher::fileChanged,this,[this]{refreshAvailability();requestRefresh();});
    m_poll.setInterval(5000);connect(&m_poll,&QTimer::timeout,this,&MusicLibrary::automaticRefresh);m_poll.start();updateWatches();
}
MusicLibrary::~MusicLibrary(){m_shuttingDown=true;m_poll.stop();m_changeTimer.stop();if(m_worker){m_worker->requestInterruption();m_worker->wait();}save();}
int MusicLibrary::count() const{return int(m_vehicle->tracks().size());}
QVariantMap MusicLibrary::current() const {return m_vehicle->tracks().value(m_vehicle->track()).toMap();}
QVariantList MusicLibrary::queue() const{
    QHash<QString,QVariantMap> indexed;const auto tracks=m_vehicle->tracks();for(int i=0;i<tracks.size();++i){auto row=tracks[i].toMap();row["index"]=i;indexed.insert(row.value("url").toString(),row);}
    QVariantList result;for(const auto &url:m_vehicle->playQueue()){auto row=indexed.value(url);row["current"]=url==m_vehicle->trackUrl();result<<row;}return result;
}
QString MusicLibrary::queueText() const{return m_vehicle->queueLabel()+QStringLiteral(" · %1 首").arg(m_vehicle->playQueue().size());}
QVariantList MusicLibrary::rootEntries() const{
    QVariantList result;for(const auto &root:m_roots){int count=0;for(const auto &entry:m_vehicle->tracks()){const QUrl url(entry.toMap().value("url").toString());if(url.isLocalFile()&&insideRoot(url.toLocalFile(),root))++count;}
        result<<QVariantMap{{"path",root},{"available",QFileInfo(root).isDir()},{"count",count}};}return result;
}
void MusicLibrary::playFromView(int index){
    QStringList urls;bool found=false;for(const auto &entry:rows()){const auto row=entry.toMap();urls<<row.value("url").toString();found|=row.value("index").toInt()==index;}
    if(!found)return;const auto row=m_vehicle->tracks().value(index).toMap();if(!row.value("available",true).toBool())return;
    QString label=m_category=="favorites"?QStringLiteral("我的收藏"):m_category=="artist"?QStringLiteral("歌手"):m_category=="album"?QStringLiteral("专辑"):m_category=="folder"?QStringLiteral("文件夹"):QStringLiteral("全部歌曲");
    if(!m_group.isEmpty())label+=" · "+(m_category=="folder"?QFileInfo(m_group).fileName():m_group);if(!m_query.trimmed().isEmpty())label=QStringLiteral("搜索结果 · ")+m_query.trimmed();
    m_vehicle->setPlayQueue(urls,label);m_vehicle->selectTrack(index);
}
void MusicLibrary::playQueued(const QString &url){for(int i=0;i<m_vehicle->tracks().size();++i)if(m_vehicle->tracks()[i].toMap().value("url")==url){m_vehicle->selectTrack(i);return;}}
void MusicLibrary::setQuery(const QString &value){if(m_query==value)return;m_query=value;emit changed();}
void MusicLibrary::setCategory(const QString &value){if(!QStringList{"all","favorites","artist","album","folder"}.contains(value)||m_category==value)return;m_category=value;m_group.clear();emit changed();}
void MusicLibrary::setGroup(const QString &value){if(m_group==value)return;m_group=value;emit changed();}
void MusicLibrary::setSort(const QString &value){if(!QStringList{"title","artist","added"}.contains(value)||m_sort==value)return;m_sort=value;emit changed();}
QStringList MusicLibrary::groups() const{
    QSet<QString> groups;const QString field=m_category=="artist"?"artist":m_category=="album"?"album":"folder";
    for(const auto &entry:m_vehicle->tracks()){
        const auto row=entry.toMap();QString value=row.value(field).toString();if(value.isEmpty())value=m_category=="artist"?QStringLiteral("未知歌手"):m_category=="album"?QStringLiteral("未知专辑"):QStringLiteral("内置音源");
        groups.insert(value);
        if(m_category=="folder"&&!row.value("folder").toString().isEmpty())for(const auto &root:m_roots)if(musicPathKey(value).startsWith(musicPathKey(root)+"/")||musicPathKey(value)==musicPathKey(root)){
            QDir dir(value);while(musicPathKey(dir.absolutePath())!=musicPathKey(root)&&dir.cdUp()){groups.insert(dir.absolutePath());if(!musicPathKey(dir.absolutePath()).startsWith(musicPathKey(root)))break;}
        }
    }
    auto list=groups.values();std::sort(list.begin(),list.end(),[](const QString &a,const QString &b){return QString::localeAwareCompare(a,b)<0;});return list;
}
QVariantList MusicLibrary::rows() const{
    QVariantList rows;const auto tracks=m_vehicle->tracks();
    for(int i=0;i<tracks.size();++i){auto row=tracks[i].toMap();row["index"]=i;
        if(m_category=="favorites"&&!row.value("favorite").toBool())continue;
        if(!m_group.isEmpty()&&QStringList{"artist","album","folder"}.contains(m_category)){
            auto value=row.value(m_category).toString();if(value.isEmpty())value=m_category=="artist"?QStringLiteral("未知歌手"):m_category=="album"?QStringLiteral("未知专辑"):QStringLiteral("内置音源");
            if(m_category=="folder"&&value!=QStringLiteral("内置音源")){const auto key=musicPathKey(value),group=musicPathKey(m_group);if(key!=group&&!key.startsWith(group+"/"))continue;}
            else if(value!=m_group)continue;
        }
        if(!m_query.trimmed().isEmpty()&&!(row.value("title").toString()+" "+row.value("artist").toString()+" "+row.value("album").toString()+" "+row.value("folder").toString()).contains(m_query.trimmed(),Qt::CaseInsensitive))continue;
        rows<<row;
    }
    if(m_sort!="added")std::stable_sort(rows.begin(),rows.end(),[&](const QVariant &a,const QVariant &b){const auto x=a.toMap(),y=b.toMap();const QString field=m_sort=="artist"?"artist":"title";const int order=QString::localeAwareCompare(x.value(field).toString(),y.value(field).toString());return order==0?x.value("index").toInt()<y.value("index").toInt():order<0;});
    return rows;
}
void MusicLibrary::stateChanged(){
    const auto url=m_vehicle->trackUrl();const bool trackChanged=url!=m_currentUrl,rowsChanged=m_lastTracks!=m_vehicle->tracks(),playChanged=m_lastPlaying!=m_vehicle->playing();
    m_lastPlaying=m_vehicle->playing();if(rowsChanged)m_lastTracks=m_vehicle->tracks();if(trackChanged){m_currentUrl=url;reloadLyrics();}
    if(rowsChanged||trackChanged)emit changed();if(trackChanged)QTimer::singleShot(0,this,[this]{updateWatches();});if(!m_loading&&(rowsChanged||trackChanged||playChanged)&&!m_saveTimer.isActive())m_saveTimer.start();
}
void MusicLibrary::updateLyricPosition(){const auto index=lyricIndexAt(m_lyrics.lines,qint64(m_vehicle->position())-lyricOffset());if(index!=m_lyricIndex){m_lyricIndex=index;emit lyricPositionChanged();}}
QString MusicLibrary::lyricsText() const {return !m_lyrics.error.isEmpty()?m_lyrics.error:!m_lyrics.plainText.isEmpty()?m_lyrics.plainText:QStringLiteral("暂无歌词\n将同名 .lrc 放在音乐文件旁，再点刷新歌词");}
void MusicLibrary::reloadLyrics(){m_lyricsStamp=lyricsStamp(m_currentUrl);m_lyrics=QUrl(m_currentUrl).isLocalFile()?loadLyrics(QUrl(m_currentUrl).toLocalFile()):LyricsDocument{};m_lyricIndex=-1;updateLyricPosition();emit lyricsChanged();}
void MusicLibrary::refreshLyricsIfChanged(){if(lyricsStamp(m_currentUrl)!=m_lyricsStamp)reloadLyrics();}
void MusicLibrary::setLyricOffset(int value){value=std::clamp(value,-10000,10000);if(lyricOffset()==value)return;m_offsets[m_currentUrl]=value;updateLyricPosition();emit lyricsChanged();m_saveTimer.start();}
void MusicLibrary::seekLyric(int index){if(index>=0&&index<m_lyrics.lines.size())m_vehicle->seek(qMax(0LL,m_lyrics.lines[index].toMap().value("at").toLongLong()+lyricOffset()));}
void MusicLibrary::toggleFavorite(int index){const auto row=m_vehicle->tracks().value(index).toMap();if(row.isEmpty())return;m_vehicle->updateTrackProperties(row.value("url").toString(),{{"favorite",!row.value("favorite").toBool()}});}
void MusicLibrary::remove(int index){if(index<3)return;const auto row=m_vehicle->tracks().value(index).toMap();if(row.isEmpty())return;const auto key=musicPathKey(QUrl(row.value("url").toString()).toLocalFile());m_excluded.insert(key);m_manualFiles.remove(key);m_vehicle->removeMusicTrack(index);updateWatches();save();}
void MusicLibrary::relocate(int index,const QUrl &url){
    if(scanning()){emit notice(QStringLiteral("扫描完成或停止后再重新定位"));return;}
    if(index<3||index>=m_vehicle->tracks().size()||!url.isLocalFile()||!supportedMusicFile(url.toLocalFile())||!QFileInfo(url.toLocalFile()).isFile()){emit notice(QStringLiteral("请选择有效的音乐文件"));return;}
    const auto key=musicPathKey(url.toLocalFile());for(int i=0;i<m_vehicle->tracks().size();++i)if(i!=index&&QUrl(m_vehicle->tracks()[i].toMap().value("url").toString()).isLocalFile()&&musicPathKey(QUrl(m_vehicle->tracks()[i].toMap().value("url").toString()).toLocalFile())==key){emit notice(QStringLiteral("该文件已经在音乐库中"));return;}
    const auto old=m_vehicle->tracks()[index].toMap();const bool wasCurrent=index==m_vehicle->track();const QFileInfo file(url.toLocalFile());QVariantMap row{{"url",url.toString()},{"title",file.completeBaseName()},{"folder",file.absolutePath()},{"available",true},{"source",QStringLiteral("本地文件")},{"cover",sidecarCover(file.absoluteFilePath())},{"favorite",old.value("favorite")}};
    const auto oldKey=musicPathKey(QUrl(old.value("url").toString()).toLocalFile());if(oldKey!=key)m_excluded.insert(oldKey);m_manualFiles.remove(oldKey);
    m_vehicle->replaceMusicTrack(index,row);m_excluded.remove(key);m_manualFiles.insert(key);if(wasCurrent)reloadLyrics();save();scan({file.absoluteFilePath()},false);
}
void MusicLibrary::metadataChanged(){
    if(m_player->source()!=QUrl(m_vehicle->trackUrl()))return;const auto tags=m_player->metaData();QVariantMap properties;
    const auto title=tags.stringValue(QMediaMetaData::Title).trimmed();if(!title.isEmpty())properties["title"]=title.left(1024);
    const auto artist=tags.stringValue(QMediaMetaData::ContributingArtist).trimmed();if(!artist.isEmpty())properties["artist"]=artist.left(1024);
    const auto album=tags.stringValue(QMediaMetaData::AlbumTitle).trimmed();if(!album.isEmpty())properties["album"]=album.left(1024);
    const auto image=qvariant_cast<QImage>(tags.value(QMediaMetaData::CoverArtImage));
    if(!image.isNull()&&!m_cache.isEmpty()&&image.width()*qint64(image.height())<=40000000){QDir().mkpath(m_cache);const QString path=m_cache+"/playing-"+QCryptographicHash::hash(m_vehicle->trackUrl().toUtf8(),QCryptographicHash::Sha256).toHex()+".jpg";if(image.scaled(512,512,Qt::KeepAspectRatio,Qt::SmoothTransformation).save(path,"JPG"))properties["cover"]=QUrl::fromLocalFile(path).toString();}
    if(!properties.isEmpty())m_vehicle->updateTrackProperties(m_vehicle->trackUrl(),properties);
}
void MusicLibrary::flushMetadata(){metadataChanged();}
void MusicLibrary::load(){
    if(!m_persist)return;QFile file(m_storage);if(!file.exists())return;
    if(file.size()>32*1024*1024||!file.open(QIODevice::ReadOnly)){m_scanText=QStringLiteral("音乐库无法读取，本次不覆盖原记录");m_persist=false;return;}
    QJsonParseError error;const auto document=QJsonDocument::fromJson(file.readAll(),&error);
    if(error.error!=QJsonParseError::NoError||!document.isObject()||document.object().value("version").toInt()!=1){m_scanText=QStringLiteral("音乐库格式异常，本次不覆盖原记录");m_persist=false;return;}
    const auto object=document.object();for(const auto &root:object.value("roots").toArray())if(root.isString()&&QDir::isAbsolutePath(root.toString()))m_roots<<QDir::cleanPath(root.toString());
    for(const auto &path:object.value("excluded").toArray())m_excluded.insert(path.toString());m_offsets=object.value("offsets").toObject().toVariantMap();
    QVariantList tracks;QSet<QString> seen;for(const auto &entry:object.value("tracks").toArray()){
        auto row=entry.toObject().toVariantMap();const QUrl url(row.value("url").toString());if(!url.isLocalFile()||!QDir::isAbsolutePath(url.toLocalFile())||!supportedMusicFile(url.toLocalFile()))continue;
        const auto key=musicPathKey(url.toLocalFile());if(seen.contains(key))continue;seen.insert(key);row["available"]=QFileInfo(url.toLocalFile()).isFile();row["source"]=QStringLiteral("本地文件");row["folder"]=QFileInfo(url.toLocalFile()).absolutePath();tracks<<row;
    }
    m_vehicle->mergeMusicTracks(tracks);for(const auto &entry:object.value("demoFavorites").toArray())m_vehicle->updateTrackProperties(entry.toString(),{{"favorite",true}});
    for(const auto &entry:object.value("manualFiles").toArray())m_manualFiles.insert(entry.toString());
    if(!object.contains("manualFiles"))for(const auto &entry:tracks){const auto path=QUrl(entry.toMap().value("url").toString()).toLocalFile();bool owned=false;for(const auto &root:m_roots)owned|=insideRoot(path,root);if(!owned)m_manualFiles.insert(musicPathKey(path));}
    if(object.contains("queue")&&object.value("queueExplicit").toBool(true)){QStringList urls;for(const auto &entry:object.value("queue").toArray())urls<<entry.toString();m_vehicle->setPlayQueue(urls,object.value("queueLabel").toString(QStringLiteral("恢复的队列")));}
    m_vehicle->restoreMusicPosition(object.value("lastUrl").toString(),qint64(object.value("position").toDouble()));
}
void MusicLibrary::save(){
    if(!m_persist||m_loading||m_vehicle->demoActive())return;QJsonArray tracks,favorites;for(const auto &entry:m_vehicle->tracks()){const auto row=entry.toMap();if(QUrl(row.value("url").toString()).isLocalFile())tracks.append(QJsonObject::fromVariantMap(row));else if(row.value("favorite").toBool())favorites.append(row.value("url").toString());}
    QJsonObject object{{"version",1},{"tracks",tracks},{"roots",QJsonArray::fromStringList(m_roots)},{"manualFiles",QJsonArray::fromStringList(m_manualFiles.values())},{"queue",QJsonArray::fromStringList(m_vehicle->playQueue())},{"queueExplicit",m_vehicle->hasExplicitQueue()},{"queueLabel",m_vehicle->queueLabel()},{"excluded",QJsonArray::fromStringList(m_excluded.values())},{"demoFavorites",favorites},{"lastUrl",m_vehicle->trackUrl()},{"position",m_vehicle->position()},{"offsets",QJsonObject::fromVariantMap(m_offsets)}};
    QDir().mkpath(QFileInfo(m_storage).absolutePath());QSaveFile file(m_storage);if(!file.open(QIODevice::WriteOnly)||file.write(QJsonDocument(object).toJson())<0||!file.commit()){m_scanText=QStringLiteral("音乐库保存失败，请检查目录权限和磁盘空间");emit scanChanged();}
}
