#include "MusicLibrary.h"
#include "MusicFiles.h"
#include "VehicleState.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QScopeGuard>
#ifdef Q_OS_WIN
#include <winrt/base.h>
#endif
namespace {
bool inside(const QString &path,const QString &root){const auto key=musicPathKey(path),base=musicPathKey(root);return key==base||key.startsWith(base.endsWith('/')?base:base+"/");}
}
void MusicLibrary::importFiles(const QVariantList &urls){
    if(scanning()){emit notice(QStringLiteral("正在扫描，请等待完成或取消后再导入"));return;}
    QStringList paths;for(const auto &entry:urls){const QUrl url=entry.toUrl();if(!url.isLocalFile())continue;const QString path=url.toLocalFile();
        if(QFileInfo(path).isDir()){const auto root=QFileInfo(path).canonicalFilePath();bool exists=false;for(const auto &value:m_roots)exists|=musicPathKey(value)==musicPathKey(root);if(!exists)m_roots<<root;paths<<root;}
        else if(supportedMusicFile(path)&&QFileInfo(path).isFile()){paths<<path;m_excluded.remove(musicPathKey(path));m_manualFiles.insert(musicPathKey(path));}
    }
    if(!paths.isEmpty()){m_autoSuspended=false;scan(paths,false);}else emit notice(QStringLiteral("请选择支持的音频文件"));
}
void MusicLibrary::importFolder(const QUrl &url){importFiles({url});}
void MusicLibrary::refreshAvailability(){
    QStringList unavailable;
    for(const auto &entry:m_vehicle->tracks()){
        const auto row=entry.toMap();const QUrl url(row.value("url").toString());if(!url.isLocalFile())continue;
        const bool available=QFileInfo(url.toLocalFile()).isFile();if(row.value("available",true).toBool()==available)continue;
        if(!available&&url==QUrl(m_vehicle->trackUrl())){m_vehicle->setPlaying(false);unavailable<<row.value("title").toString();}
        m_vehicle->updateTrackProperties(url.toString(),{{"available",available}});
    }
    if(!unavailable.isEmpty())emit notice(QStringLiteral("当前音源已断开，播放已暂停；恢复文件后请手动播放"));
    QString signature;for(const auto &root:m_roots)signature+=root+(QFileInfo(root).isDir()?"1":"0");
    if(signature!=m_rootSignature){m_rootSignature=signature;emit changed();}
}
void MusicLibrary::requestRefresh(){
    if(m_shuttingDown)return;m_autoSuspended=false;refreshAvailability();m_changeTimer.start();
}
void MusicLibrary::automaticRefresh(){
    if(m_shuttingDown)return;refreshAvailability();if(m_autoSuspended)return;
    if(scanning()){m_rescanPending=true;return;}
    QStringList paths=m_roots;for(const auto &entry:m_vehicle->tracks()){const QUrl url(entry.toMap().value("url").toString());if(url.isLocalFile()&&QFileInfo::exists(url.toLocalFile()))paths<<url.toLocalFile();}
    if(!paths.isEmpty())scan(paths,true);else updateWatches();
}
void MusicLibrary::rescan(){
    if(scanning()){m_rescanPending=true;return;}m_autoSuspended=false;refreshAvailability();
    QStringList paths=m_roots;for(const auto &entry:m_vehicle->tracks()){const QUrl url(entry.toMap().value("url").toString());if(url.isLocalFile()&&QFileInfo::exists(url.toLocalFile()))paths<<url.toLocalFile();}
    if(!paths.isEmpty())scan(paths,false);else emit notice(QStringLiteral("还没有已导入的音乐目录"));reloadLyrics();
}
void MusicLibrary::updateWatches(const QStringList &directories){
    if(!directories.isEmpty())m_directories=directories;QSet<QString> desired;
    auto addParent=[&](QString path){QFileInfo info(path);if(!info.isDir())path=info.absolutePath();for(int i=0;i<32&&!QFileInfo(path).isDir();++i){const auto parent=QFileInfo(path).absolutePath();if(parent==path)break;path=parent;}if(QFileInfo(path).isDir())desired.insert(path);};
    for(const auto &root:m_roots){if(QFileInfo(root).isDir())desired.insert(root);addParent(QFileInfo(root).absolutePath());}
    for(const auto &entry:m_vehicle->tracks()){const QUrl url(entry.toMap().value("url").toString());if(url.isLocalFile())addParent(QFileInfo(url.toLocalFile()).absolutePath());}
    for(const auto &dir:m_directories)if(desired.size()<1024&&QFileInfo(dir).isDir())desired.insert(dir);
    const QUrl current(m_vehicle->trackUrl());if(current.isLocalFile()){
        const auto path=current.toLocalFile();QFileInfo file(path);const QString base=file.absolutePath()+"/"+file.completeBaseName();
        for(const auto &candidate:QStringList{path,base+".lrc",base+".txt",QUrl(sidecarCover(path)).toLocalFile()})if(QFileInfo(candidate).isFile())desired.insert(candidate);
    }
#ifdef Q_OS_WIN
    // Qt's native watches on descendants can prevent an ancestor from being
    // renamed on Windows. Keep only outer directory watches; the periodic scan
    // covers deeper changes without retaining handles inside the music roots.
    QMap<QString,QString> candidates;for(const auto &path:desired)if(QFileInfo(path).isDir())candidates.insert(musicPathKey(path),path);
    desired.clear();QString lastParent;
    for(auto it=candidates.cbegin();it!=candidates.cend();++it){
        if(!lastParent.isEmpty()&&it.key().startsWith(lastParent.endsWith('/')?lastParent:lastParent+"/"))continue;
        desired.insert(it.value());lastParent=it.key();
    }
#endif
    const auto watched=m_watcher.directories()+m_watcher.files();QStringList remove,add;for(const auto &path:watched)if(!desired.contains(path))remove<<path;for(const auto &path:desired)if(!watched.contains(path))add<<path;
    if(!remove.isEmpty())m_watcher.removePaths(remove);if(!add.isEmpty())m_watcher.addPaths(add);
}
void MusicLibrary::scan(const QStringList &paths,bool automatic){
    if(scanning()){m_rescanPending=true;return;}
    struct Result {QVariantList rows;QStringList directories;int skipped=0,decoded=0;bool cancelled=false;};auto result=std::make_shared<Result>();
    QHash<QString,QVariantMap> cached;for(const auto &entry:m_vehicle->tracks()){const auto row=entry.toMap();const QUrl url(row.value("url").toString());if(url.isLocalFile())cached.insert(musicPathKey(url.toLocalFile()),row);}
    if(!automatic)m_scanText=QStringLiteral("正在扫描音乐…");const auto cache=m_cache;const auto excluded=m_excluded;
    auto *thread=QThread::create([this,result,paths,cache,excluded,cached,automatic]{
#ifdef Q_OS_WIN
        bool apartment=false;try{winrt::init_apartment(winrt::apartment_type::multi_threaded);apartment=true;}catch(...){}
        auto cleanup=qScopeGuard([&]{if(apartment)winrt::uninit_apartment();});
#endif
        QSet<QString> visited,seenDirs;int processed=0;
        auto add=[&](const QString &path){
            if(QThread::currentThread()->isInterruptionRequested())return;const auto key=musicPathKey(path);if(visited.contains(key)||excluded.contains(key)||!supportedMusicFile(path))return;visited.insert(key);
            if(!QFileInfo(path).isReadable()){++result->skipped;return;}
            auto row=cached.value(key);if(!row.isEmpty()&&row.value("_stamp").toString()==musicFileStamp(path)){row["available"]=true;result->rows<<row;}
            else{result->rows<<readMusicFile(path,cache);++result->decoded;}++processed;
            if(!automatic&&processed%10==0)QMetaObject::invokeMethod(this,[this,processed]{m_scanText=QStringLiteral("已扫描 %1 首…").arg(processed);emit scanChanged();},Qt::QueuedConnection);
        };
        for(const auto &path:paths){if(QThread::currentThread()->isInterruptionRequested())break;if(QFileInfo(path).isDir()){
            const auto rootKey=musicPathKey(path);if(seenDirs.contains(rootKey))continue;seenDirs.insert(rootKey);result->directories<<QFileInfo(path).absoluteFilePath();
            QDirIterator entries(path,QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot|QDir::NoSymLinks,QDirIterator::Subdirectories);
            while(entries.hasNext()&&!QThread::currentThread()->isInterruptionRequested()){
                const auto entry=entries.next();if(entries.fileInfo().isDir()){const auto key=musicPathKey(entry);if(!seenDirs.contains(key)){seenDirs.insert(key);result->directories<<entry;}}
                else add(entry);if(visited.size()>=50000){result->cancelled=true;break;}
            }
        }else if(QFileInfo(path).isFile())add(path);if(visited.size()>=50000)break;}
        result->cancelled|=QThread::currentThread()->isInterruptionRequested();
    });m_worker=thread;thread->setParent(this);
    connect(thread,&QThread::finished,this,[this,result,thread,automatic]{
        m_worker=nullptr;m_loading=true;QHash<QString,QVariantMap> existing;
        for(const auto &entry:m_vehicle->tracks()){const auto row=entry.toMap();const QUrl url(row.value("url").toString());if(url.isLocalFile())existing.insert(musicPathKey(url.toLocalFile()),row);}
        QVariantList updates;for(auto &entry:result->rows){auto row=entry.toMap();const auto path=QUrl(row.value("url").toString()).toLocalFile();const auto old=existing.value(musicPathKey(path));
            if(!old.isEmpty()){row["url"]=old.value("url");for(const auto &key:QStringList{"artist","album","cover"})if(row.value(key).toString().isEmpty()&&!old.value(key).toString().isEmpty())row[key]=old.value(key);if(row.value("title")==QFileInfo(path).completeBaseName()&&!old.value("title").toString().isEmpty())row["title"]=old.value("title");}
            row["available"]=QFileInfo(path).isFile();if(old.isEmpty()||old!=row)updates<<row;
        }
        if(!updates.isEmpty())m_vehicle->mergeMusicTracks(updates);m_loading=false;refreshAvailability();updateWatches(result->directories);refreshLyricsIfChanged();
        if(!automatic||!updates.isEmpty()||result->cancelled)m_scanText=(result->cancelled?QStringLiteral("扫描已停止，已处理 %1 首"):automatic?QStringLiteral("音乐库已自动更新 %1 首"):QStringLiteral("扫描完成，已处理 %1 首")).arg(automatic?updates.size():result->rows.size());
        if(result->skipped&&!automatic)m_scanText+=QStringLiteral("，%1 个文件不可读").arg(result->skipped);
        thread->deleteLater();emit scanChanged();if(!updates.isEmpty()||!automatic)emit changed();if(!updates.isEmpty()||!automatic)save();if(!automatic)emit notice(m_scanText);
        if(m_rescanPending&&!m_autoSuspended){m_rescanPending=false;m_changeTimer.start();}
    });emit scanChanged();if(!automatic)emit changed();thread->start();
}
void MusicLibrary::cancelScan(){if(m_worker){m_autoSuspended=true;m_rescanPending=false;m_worker->requestInterruption();m_scanText=QStringLiteral("正在停止扫描…");emit scanChanged();}}
void MusicLibrary::removeRoot(const QString &path){
    if(scanning()){emit notice(QStringLiteral("扫描完成或停止后再移除目录"));return;}
    int at=-1;for(int i=0;i<m_roots.size();++i)if(musicPathKey(m_roots[i])==musicPathKey(path))at=i;if(at<0)return;
    const auto removed=m_roots.takeAt(at);int count=0;
    for(int i=m_vehicle->tracks().size()-1;i>=3;--i){const QUrl url(m_vehicle->tracks()[i].toMap().value("url").toString());if(!url.isLocalFile()||!inside(url.toLocalFile(),removed)||m_manualFiles.contains(musicPathKey(url.toLocalFile())))continue;
        bool shared=false;for(const auto &root:m_roots)shared|=inside(url.toLocalFile(),root);if(!shared){m_vehicle->removeMusicTrack(i);++count;}}
    m_directories.clear();for(const auto &root:m_roots)m_directories<<root;m_group.clear();m_rescanPending=false;updateWatches();save();emit changed();emit notice(QStringLiteral("已移除目录及 %1 个独占条目，原文件保留").arg(count));
}
