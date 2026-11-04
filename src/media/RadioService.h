#pragma once
#include <QAbstractListModel>
#include <QTimer>
#include <QSet>
#include "RadioSourceResolver.h"
class MediaSession;

class RadioService : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY changed)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY changed)
    Q_PROPERTY(QString currentId READ currentId NOTIFY changed)
    Q_PROPERTY(QString currentName READ currentName NOTIFY changed)
    Q_PROPERTY(QString currentCategory READ currentCategory NOTIFY changed)
    Q_PROPERTY(bool currentFavorite READ currentFavorite NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(bool active READ active NOTIFY changed)
  public:
    enum Role { Id = Qt::UserRole + 1, Name, Category, Favorite, Selected };
    explicit RadioService(MediaSession *, QObject *parent = nullptr);
    ~RadioService() override;
    int rowCount(const QModelIndex &parent = {}) const override {
        return parent.isValid() ? 0 : m_rows.size();
    }
    QVariant data(const QModelIndex &, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    QString query() const { return m_query; }
    QString filter() const { return m_filter; }
    QString currentId() const { return m_current; }
    QString currentName() const;
    QString currentCategory() const;
    bool currentFavorite() const { return m_favorites.contains(m_current); }
    int count() const { return m_rows.size(); }
    bool active() const;
    QString stationIdForName(const QString &name, bool favoriteOnly) const;
    void setQuery(const QString &);
    void setFilter(const QString &);
    Q_INVOKABLE void tune(const QString &);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void toggle();
    Q_INVOKABLE void next(int direction = 1);
    Q_INVOKABLE void toggleFavorite(const QString &);
  signals:
    void changed();

  private:
    struct Station {
        QString id, name, category, channel;
        RadioSourceResolver::Provider provider;
    };
    void refresh();
    void resolve();
    void interrupted(quint64, const QString &);
    void save();
    void load();
    const Station *find(const QString &) const;
    MediaSession *m_session;
    RadioSourceResolver m_resolver;
    QTimer m_retry;
    QVector<Station> m_stations;
    QVector<int> m_rows;
    QSet<QString> m_favorites;
    QStringList m_recent;
    QString m_current = "cnr-1", m_query, m_filter = "all", m_storage;
    quint64 m_ticket = 0;
    int m_attempts = 0;
    bool m_wanted = false, m_persist = false;
};
