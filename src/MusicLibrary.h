#pragma once
#include <QObject>
#include <QThread>
#include <QTimer>
#include <QVariantList>
#include <QSet>
#include <QFileSystemWatcher>
#include <QMediaPlayer>
#include "Lyrics.h"
class MediaState;
class MusicLibrary:public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList rows READ rows NOTIFY changed)
    Q_PROPERTY(QStringList groups READ groups NOTIFY changed)
    Q_PROPERTY(QStringList roots READ roots NOTIFY changed)
    Q_PROPERTY(QVariantList rootEntries READ rootEntries NOTIFY changed)
    Q_PROPERTY(QVariantList queue READ queue NOTIFY changed)
    Q_PROPERTY(int queueCount READ queueCount NOTIFY changed)
    Q_PROPERTY(QString queueText READ queueText NOTIFY changed)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY changed)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY changed)
    Q_PROPERTY(QString group READ group WRITE setGroup NOTIFY changed)
    Q_PROPERTY(QString sort READ sort WRITE setSort NOTIFY changed)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanChanged)
    Q_PROPERTY(QString scanText READ scanText NOTIFY scanChanged)
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
    Q_PROPERTY(QVariantList lyrics READ lyrics NOTIFY lyricsChanged)
    Q_PROPERTY(QString lyricsText READ lyricsText NOTIFY lyricsChanged)
    Q_PROPERTY(int lyricIndex READ lyricIndex NOTIFY lyricPositionChanged)
    Q_PROPERTY(int lyricOffset READ lyricOffset WRITE setLyricOffset NOTIFY lyricsChanged)
public:
    MusicLibrary(MediaState*,QMediaPlayer*,QObject *parent=nullptr,const QString &storageFile={});
    ~MusicLibrary() override;
    QVariantList rows() const;
    QStringList groups() const;
    QStringList roots() const{return m_roots;}
    QVariantList rootEntries() const;
    QVariantList queue() const;
    int queueCount() const;
    QString queueText() const;
    QString query() const{return m_query;}
    QString category() const{return m_category;}
    QString group() const{return m_group;}
    QString sort() const{return m_sort;}
    bool scanning() const{return m_worker!=nullptr;}
    QString scanText() const{return m_scanText;}
    int count() const;
    QVariantMap current() const;
    QVariantList lyrics() const{return m_lyrics.lines;}
    QString lyricsText() const;
    int lyricIndex() const{return m_lyricIndex;}
    int lyricOffset() const{return m_offsets.value(m_currentUrl).toInt();}
    void setQuery(const QString&);
    void setCategory(const QString&);
    void setGroup(const QString&);
    void setSort(const QString&);
    void setLyricOffset(int);
    Q_INVOKABLE void importFiles(const QVariantList&);
    Q_INVOKABLE void importFolder(const QUrl&);
    Q_INVOKABLE void rescan();
    Q_INVOKABLE void removeRoot(const QString &path);
    Q_INVOKABLE void playFromView(int index);
    Q_INVOKABLE void playQueued(const QString &url);
    Q_INVOKABLE void cancelScan();
    Q_INVOKABLE void toggleFavorite(int index);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void relocate(int index,const QUrl&);
    Q_INVOKABLE void seekLyric(int index);
    Q_INVOKABLE void reloadLyrics();
    void save();
    void flushMetadata();
signals:
    void changed();
    void scanChanged();
    void lyricsChanged();
    void lyricPositionChanged();
    void notice(const QString&);
private:
    void scan(const QStringList &paths,bool automatic);
    void automaticRefresh();
    void requestRefresh();
    void refreshAvailability();
    void updateWatches(const QStringList &directories={});
    void stateChanged();
    void updateLyricPosition();
    void refreshLyricsIfChanged();
    void load();
    void metadataChanged();
    MediaState *m_state;
    QMediaPlayer *m_player;
    QThread *m_worker=nullptr;
    QTimer m_saveTimer,m_checkpoint;
    QTimer m_changeTimer,m_poll;
    QFileSystemWatcher m_watcher;
    QString m_storage,m_cache,m_query,m_group,m_category="all",m_sort="title",m_scanText,m_currentUrl,m_rootSignature;
    QString m_lyricsStamp;
    QStringList m_roots;
    QSet<QString> m_excluded;
    QSet<QString> m_manualFiles;
    QStringList m_directories;
    QVariantMap m_offsets;
    LyricsDocument m_lyrics;
    int m_lyricIndex=-1;
    bool m_loading=false,m_persist=false;
    QVariantList m_lastTracks;
    bool m_lastPlaying=false;
    bool m_rescanPending=false,m_autoSuspended=false,m_shuttingDown=false;
};
