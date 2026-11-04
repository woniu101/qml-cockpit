#include "WeatherService.h"
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QSettings>
#include <QUrlQuery>
#include <cmath>

namespace {
bool validCoordinates(const QVariantMap &location)
{
    bool latOk = false, lonOk = false;
    const double lat = location.value("latitude").toDouble(&latOk);
    const double lon = location.value("longitude").toDouble(&lonOk);
    return latOk && lonOk && std::abs(lat) <= 90 && std::abs(lon) <= 180;
}

QString weatherIcon(int code, bool day)
{
    if (code == 0) return day ? "sun" : "moon";
    if ((code >= 71 && code <= 77) || code == 85 || code == 86) return "snow";
    if (code >= 51) return "rain";
    return "cloud";
}

QVariantMap cityCandidate(const QJsonObject &result, const char *cityField, const char *regionField)
{
    const auto name = result.value(cityField).toString().trimmed();
    if (name.isEmpty() || !result.value("latitude").isDouble() || !result.value("longitude").isDouble())
        return {};
    QVariantMap location{{"name", name}, {"latitude", result.value("latitude").toDouble()},
                         {"longitude", result.value("longitude").toDouble()}};
    if (!validCoordinates(location))
        return {};
    QStringList label{name};
    for (const auto *field : {regionField, "country"}) {
        const auto part = result.value(field).toString().trimmed();
        if (!part.isEmpty() && !label.contains(part))
            label << part;
    }
    location.insert("label", label.join(" · "));
    return location;
}
}

WeatherService::WeatherService(bool enabled) : m_enabled(enabled)
{
    if (!m_enabled)
        return;
    const auto saved = QSettings().value("weather/city").toMap();
    if (validCoordinates(saved) && !saved.value("name").toString().isEmpty())
        m_location = saved;
    connect(&m_timer, &QTimer::timeout, this, &WeatherService::refresh);
    m_timer.start(15 * 60 * 1000);
    if (!m_location.isEmpty())
        QTimer::singleShot(0, this, &WeatherService::refresh);
}

QString WeatherService::city() const
{
    return m_location.value("name").toString();
}

void WeatherService::get(const QUrl &url, std::function<void(const QJsonObject &)> done)
{
    QNetworkRequest request(url);
    request.setTransferTimeout(12000);
    auto *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, done = std::move(done)] {
        reply->deleteLater();
        done(reply->error() == QNetworkReply::NoError
                 ? QJsonDocument::fromJson(reply->readAll()).object() : QJsonObject());
    });
}

void WeatherService::search(const QString &query)
{
    const auto name = query.trimmed().left(80);
    if (!m_enabled || m_searching || name.size() < 2)
        return;
    m_searching = true;
    m_matches.clear();
    m_searchStatus = QStringLiteral("正在搜索城市…");
    emit searchChanged();

    QUrl url("https://geocoding-api.open-meteo.com/v1/search");
    QUrlQuery parameters;
    parameters.addQueryItem("name", name);
    parameters.addQueryItem("language", "zh");
    parameters.addQueryItem("count", "6");
    url.setQuery(parameters);
    get(url, [this](const QJsonObject &response) {
        for (const auto &value : response.value("results").toArray()) {
            const auto candidate = cityCandidate(value.toObject(), "name", "admin1");
            if (!candidate.isEmpty())
                m_matches << candidate;
        }
        m_searching = false;
        m_searchStatus = response.isEmpty() ? QStringLiteral("城市查询失败，请检查网络后重试")
                         : m_matches.isEmpty() ? QStringLiteral("未找到城市，可尝试拼音")
                                               : QStringLiteral("请选择匹配的城市");
        emit searchChanged();
    });
}

void WeatherService::selectCity(int index)
{
    if (index < 0 || index >= m_matches.size())
        return;
    applyCity(m_matches.at(index).toMap());
}

void WeatherService::locateByIp()
{
    if (!m_enabled || m_locating)
        return;
    m_locating = true;
    m_locationFailed = false;
    m_suggestion.clear();
    emit suggestionChanged();

    QUrl url("https://ipwho.is/");
    QUrlQuery parameters;
    parameters.addQueryItem("lang", "zh-CN");
    parameters.addQueryItem("fields", "success,city,region,country,latitude,longitude");
    url.setQuery(parameters);
    get(url, [this](const QJsonObject &response) {
        if (response.value("success").toBool())
            m_suggestion = cityCandidate(response, "city", "region");
        m_locationFailed = m_suggestion.isEmpty();
        m_locating = false;
        emit suggestionChanged();
    });
}

void WeatherService::selectSuggestedCity()
{
    if (!m_suggestion.isEmpty())
        applyCity(m_suggestion);
}

void WeatherService::applyCity(const QVariantMap &location)
{
    if (m_location == location) {
        refresh();
        return;
    }
    m_location = location;
    QSettings().setValue("weather/city", m_location);
    ++m_cityRevision;
    m_temperature.clear();
    m_observedAt.clear();
    m_icon = "nav";
    m_refreshing = false;
    refresh();
}

void WeatherService::refresh()
{
    if (!m_enabled || m_location.isEmpty() || m_refreshing)
        return;
    m_refreshing = true;
    m_status = QStringLiteral("正在更新天气…");
    emit changed();
    const auto revision = m_cityRevision;
    QUrl url("https://api.open-meteo.com/v1/forecast");
    QUrlQuery parameters;
    for (const auto *field : {"latitude", "longitude"})
        parameters.addQueryItem(field, QString::number(m_location.value(field).toDouble(), 'f', 4));
    parameters.addQueryItem("current", "temperature_2m,weather_code,is_day");
    parameters.addQueryItem("timezone", "auto");
    url.setQuery(parameters);
    get(url, [this, revision](const QJsonObject &response) {
        if (revision != m_cityRevision)
            return;
        m_refreshing = false;
        const auto current = response.value("current").toObject();
        const auto temperature = current.value("temperature_2m");
        const auto time = QDateTime::fromString(current.value("time").toString(), Qt::ISODate);
        if (temperature.isDouble() && time.isValid()) {
            m_temperature = QString::number(qRound(temperature.toDouble())) + QStringLiteral("°");
            m_icon = weatherIcon(current.value("weather_code").toInt(-1), current.value("is_day").toInt() == 1);
            m_observedAt = time.toString("MM-dd HH:mm");
            m_status = QStringLiteral("当地时间 %1").arg(m_observedAt);
        } else {
            m_status = m_observedAt.isEmpty() ? QStringLiteral("天气暂不可用，请检查网络")
                       : QStringLiteral("更新失败 · 上次天气 %1（当地时间）").arg(m_observedAt);
        }
        emit changed();
    });
}
