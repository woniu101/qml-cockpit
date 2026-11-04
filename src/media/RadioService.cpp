#include "RadioService.h"
#include "MediaSession.h"
#include <QStandardPaths>
#include <QSaveFile>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>

RadioService::RadioService(MediaSession *session, QObject *parent)
    : QAbstractListModel(parent), m_session(session) {
    using Provider = RadioSourceResolver::Provider;
    m_stations = {
        {"cnr-1", QStringLiteral("中国之声"), QStringLiteral("央广 · 新闻综合"), "1", Provider::Cnr},
        {"cnr-2", QStringLiteral("经济之声"), QStringLiteral("央广 · 财经资讯"), "2", Provider::Cnr},
        {"cnr-4", QStringLiteral("音乐之声"), QStringLiteral("央广 · 音乐"), "4", Provider::Cnr},
        {"rthk-1", QStringLiteral("香港电台第一台"), QStringLiteral("香港 · 综合"), "radio1",
         Provider::Rthk}};
    m_persist = !qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS");
    m_storage = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/media/radio.json";
    load();
    refresh();
    m_retry.setSingleShot(true);
    connect(&m_retry, &QTimer::timeout, this, &RadioService::resolve);
    connect(session, &MediaSession::sessionChanged, this, [this] {
        if (m_ticket != m_session->generation()) {
            m_wanted = false;
            m_retry.stop();
            m_resolver.cancel();
        }
        refresh();
    });
    connect(session, &MediaSession::changed, this, [this] {
        if (!m_rows.isEmpty())
            emit dataChanged(index(0), index(m_rows.size() - 1));
        emit changed();
    });
    connect(session, &MediaSession::radioInterrupted, this, &RadioService::interrupted);
    connect(session, &MediaSession::radioRestoreRequested, this, &RadioService::tune);
    connect(&m_resolver, &RadioSourceResolver::resolved, this, [this](quint64 ticket, const QUrl &url) {
        if (m_wanted && ticket == m_ticket && ticket == m_session->generation())
            m_session->openRadio(ticket, url);
    });
    connect(&m_resolver, &RadioSourceResolver::failed, this, &RadioService::interrupted);
}
RadioService::~RadioService() {
    m_retry.stop();
    m_resolver.cancel();
    save();
}
const RadioService::Station *RadioService::find(const QString &id) const {
    for (const auto &station : m_stations)
        if (station.id == id)
            return &station;
    return nullptr;
}
QString RadioService::currentName() const {
    const auto *station = find(m_current);
    return station ? station->name : QString();
}
QString RadioService::stationIdForName(const QString &name, bool favoriteOnly) const {
    for (const auto &station : m_stations)
        if (station.name == name && (!favoriteOnly || m_favorites.contains(station.id)))
            return station.id;
    return {};
}
QString RadioService::currentCategory() const {
    const auto *station = find(m_current);
    return station ? station->category : QString();
}
bool RadioService::active() const {
    return m_session->kind() == "radio" && m_session->itemId() == m_current &&
           (m_session->playing() || m_session->busy());
}
QHash<int, QByteArray> RadioService::roleNames() const {
    return {{Id, "stationId"},
            {Name, "stationName"},
            {Category, "stationCategory"},
            {Favorite, "stationFavorite"},
            {Selected, "stationSelected"}};
}
QVariant RadioService::data(const QModelIndex &at, int role) const {
    if (!at.isValid() || at.row() < 0 || at.row() >= m_rows.size())
        return {};
    const auto &station = m_stations[m_rows[at.row()]];
    switch (role) {
    case Id:
        return station.id;
    case Name:
        return station.name;
    case Category:
        return station.category;
    case Favorite:
        return m_favorites.contains(station.id);
    case Selected:
        return station.id == m_current;
    default:
        return {};
    }
}
void RadioService::refresh() {
    beginResetModel();
    m_rows.clear();
    for (int i = 0; i < m_stations.size(); ++i) {
        const auto &s = m_stations[i];
        if (m_filter == "favorites" && !m_favorites.contains(s.id))
            continue;
        if (m_filter == "recent" && !m_recent.contains(s.id))
            continue;
        if (!(s.name + s.category).contains(m_query.trimmed(), Qt::CaseInsensitive))
            continue;
        m_rows << i;
    }
    if (m_filter == "recent")
        std::sort(m_rows.begin(), m_rows.end(), [this](int a, int b) {
            return m_recent.indexOf(m_stations[a].id) < m_recent.indexOf(m_stations[b].id);
        });
    endResetModel();
    emit changed();
}
void RadioService::setQuery(const QString &value) {
    if (m_query == value)
        return;
    m_query = value.left(200);
    refresh();
}
void RadioService::setFilter(const QString &value) {
    if (!QStringList{"all", "favorites", "recent"}.contains(value) || m_filter == value)
        return;
    m_filter = value;
    refresh();
}
void RadioService::tune(const QString &id) {
    const auto *station = find(id);
    if (!station)
        return;
    m_retry.stop();
    m_resolver.cancel();
    m_wanted = false;
    const auto ticket = m_session->beginRadio(id, station->name);
    if (!ticket)
        return;
    m_current = id;
    m_ticket = ticket;
    m_attempts = 0;
    m_wanted = true;
    m_recent.removeAll(id);
    m_recent.prepend(id);
    while (m_recent.size() > 30)
        m_recent.removeLast();
    save();
    refresh();
    resolve();
}
void RadioService::resolve() {
    if (!m_wanted || m_ticket != m_session->generation())
        return;
    const auto *station = find(m_current);
    if (station)
        m_resolver.resolve(station->provider, station->channel, m_ticket);
}
void RadioService::interrupted(quint64 ticket, const QString &message) {
    if (ticket != m_ticket || ticket != m_session->generation() || !m_wanted || m_retry.isActive())
        return;
    m_resolver.cancel();
    const bool retry = m_attempts < 3;
    m_session->radioFailure(ticket, message, retry);
    if (retry)
        m_retry.start(1000 * (1 << m_attempts++));
    else
        m_wanted = false;
}
void RadioService::stop() {
    m_wanted = false;
    m_retry.stop();
    m_resolver.cancel();
    if (m_session->kind() == "radio")
        m_session->stop();
    emit changed();
}
void RadioService::toggle() {
    if (active())
        stop();
    else
        tune(m_current);
}
void RadioService::next(int direction) {
    QStringList ids;
    for (const auto &station : m_stations)
        if (m_favorites.contains(station.id))
            ids << station.id;
    if (ids.isEmpty())
        for (const auto &station : m_stations)
            ids << station.id;
    int at = ids.indexOf(m_current);
    at = at < 0 ? (direction < 0 ? ids.size() - 1 : 0)
                : (at + (direction < 0 ? -1 : 1) + ids.size()) % ids.size();
    tune(ids[at]);
}
void RadioService::toggleFavorite(const QString &id) {
    if (!find(id))
        return;
    if (!m_favorites.remove(id))
        m_favorites.insert(id);
    save();
    refresh();
}
void RadioService::save() {
    if (!m_persist)
        return;
    QDir().mkpath(QFileInfo(m_storage).absolutePath());
    QSaveFile file(m_storage);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(QJsonObject{{"version", 1},
                                             {"favorites", QJsonArray::fromStringList(m_favorites.values())},
                                             {"recent", QJsonArray::fromStringList(m_recent)},
                                             {"last", m_current}})
                       .toJson());
        file.commit();
    }
}
void RadioService::load() {
    if (!m_persist)
        return;
    QFile file(m_storage);
    if (!file.exists())
        return;
    if (file.size() > 256 * 1024 || !file.open(QIODevice::ReadOnly)) {
        m_persist = false;
        return;
    }
    QJsonParseError error;
    const auto object = QJsonDocument::fromJson(file.readAll(), &error).object();
    if (error.error != QJsonParseError::NoError || object["version"].toInt() != 1) {
        m_persist = false;
        return;
    }
    for (const auto &id : object["favorites"].toArray())
        if (find(id.toString()))
            m_favorites.insert(id.toString());
    for (const auto &id : object["recent"].toArray())
        if (find(id.toString()) && !m_recent.contains(id.toString()))
            m_recent << id.toString();
    const auto last = object["last"].toString();
    if (find(last))
        m_current = last;
}
