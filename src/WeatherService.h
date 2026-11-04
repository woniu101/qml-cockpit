#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <functional>

class WeatherService final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString city READ city NOTIFY changed)
    Q_PROPERTY(QString temperature READ temperature NOTIFY changed)
    Q_PROPERTY(QString icon READ icon NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QVariantList matches READ matches NOTIFY searchChanged)
    Q_PROPERTY(QString searchStatus READ searchStatus NOTIFY searchChanged)
    Q_PROPERTY(bool searching READ searching NOTIFY searchChanged)
    Q_PROPERTY(bool refreshing READ refreshing NOTIFY changed)
    Q_PROPERTY(QString suggestedCity READ suggestedCity NOTIFY suggestionChanged)
    Q_PROPERTY(bool locating READ locating NOTIFY suggestionChanged)
    Q_PROPERTY(bool locationFailed READ locationFailed NOTIFY suggestionChanged)
public:
    explicit WeatherService(bool enabled);
    QString city() const;
    QString temperature() const { return m_temperature; }
    QString icon() const { return m_icon; }
    QString status() const { return m_status; }
    QVariantList matches() const { return m_matches; }
    QString searchStatus() const { return m_searchStatus; }
    bool searching() const { return m_searching; }
    bool refreshing() const { return m_refreshing; }
    QString suggestedCity() const { return m_suggestion.value("label").toString(); }
    bool locating() const { return m_locating; }
    bool locationFailed() const { return m_locationFailed; }

    Q_INVOKABLE void search(const QString &query);
    Q_INVOKABLE void selectCity(int index);
    Q_INVOKABLE void locateByIp();
    Q_INVOKABLE void selectSuggestedCity();
    Q_INVOKABLE void refresh();
private:
    void get(const QUrl &url, std::function<void(const QJsonObject &)> done);
    void applyCity(const QVariantMap &location);
    QTimer m_timer;
    QVariantMap m_location;
    QVariantMap m_suggestion;
    QVariantList m_matches;
    QString m_status = QStringLiteral("请选择城市");
    QString m_searchStatus, m_temperature, m_icon = "nav", m_observedAt;
    unsigned m_cityRevision = 0;
    bool m_enabled, m_searching = false, m_refreshing = false;
    bool m_locating = false, m_locationFailed = false;
    QNetworkAccessManager m_network;
signals:
    void changed();
    void searchChanged();
    void suggestionChanged();
};
