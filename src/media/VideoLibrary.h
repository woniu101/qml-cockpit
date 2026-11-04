#pragma once
#include <QAbstractListModel>
#include <QTimer>
#include <QThread>
#include <QUrl>
#include <QSet>
#include <QVariantList>
class MediaSession;

class VideoLibrary : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY changed)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY changed)
    Q_PROPERTY(QString folder READ folder WRITE setFolder NOTIFY changed)
    Q_PROPERTY(QStringList folders READ folders NOTIFY changed)
    Q_PROPERTY(QStringList roots READ roots NOTIFY changed)
    Q_PROPERTY(bool scanning READ scanning NOTIFY changed)
    Q_PROPERTY(QString scanText READ scanText NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY changed)
  public:
    enum Role { Url = Qt::UserRole + 1, Title, Folder, Cover, Duration, Bookmark, Available, LastWatched };
    explicit VideoLibrary(MediaSession *, QObject *parent = nullptr, const QString &storage = {});
    ~VideoLibrary() override;
    int rowCount(const QModelIndex &parent = {}) const override {
        return parent.isValid() ? 0 : m_rows.size();
    }
    QVariant data(const QModelIndex &, int) const override;
    QHash<int, QByteArray> roleNames() const override;
    QString query() const { return m_query; }
    QString category() const { return m_category; }
    QString folder() const { return m_folder; }
    QStringList folders() const;
    QStringList roots() const { return m_roots; }
    bool scanning() const { return m_worker; }
    QVariantList playbackItems() const;
    QString scanText() const { return m_scanText; }
    int count() const { return m_rows.size(); }
    void setQuery(const QString &);
    void setCategory(const QString &);
    void setFolder(const QString &);
    Q_INVOKABLE void importFiles(const QVariantList &);
    Q_INVOKABLE void importFolder(const QUrl &);
    Q_INVOKABLE void rescan();
    Q_INVOKABLE void cancelScan();
    Q_INVOKABLE void removeRoot(const QString &);
    Q_INVOKABLE void remove(const QString &url);
    Q_INVOKABLE void relocate(const QString &previous, const QUrl &);
    Q_INVOKABLE void play(const QString &url);
    Q_INVOKABLE void next(int direction = 1);
    void save();
  signals:
    void changed();
    void notice(const QString &);

  private:
    struct Entry {
        QString url, title, folder, cover, stamp;
        qint64 duration = 0, bookmark = 0, watched = 0;
        bool available = true;
    };
    void load();
    void refresh();
    void scan(bool automatic);
    void checkpoint(const QUrl &, qint64, qint64);
    static Entry inspect(const QString &, const QString &cache, qint64 &cacheBytesLeft);
    static bool supported(const QString &);
    MediaSession *m_session;
    QVector<Entry> m_entries;
    QVector<int> m_rows;
    QStringList m_roots;
    QSet<QString> m_manual, m_excluded;
    QString m_storage, m_cache, m_query, m_category = "all", m_folder, m_scanText;
    QThread *m_worker = nullptr;
    QTimer m_poll, m_saveTimer;
    bool m_persist = false, m_pending = false, m_autoSuspended = false;
};
