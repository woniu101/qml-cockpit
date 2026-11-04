#include "RadioSourceResolver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

void RadioSourceResolver::cancel() {
    ++m_request;
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}
void RadioSourceResolver::resolve(Provider provider, const QString &channel, quint64 ticket) {
    cancel();
    const auto requestId = m_request;
    const QUrl endpoint(provider == Provider::Cnr ? "https://apppc.cnr.cn/national"
                                                  : "https://www.rthk.hk/radio/radio1");
    QNetworkRequest request(endpoint);
    request.setTransferTimeout(10000);
    request.setRawHeader("User-Agent", "QmlCockpit/0.2");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    auto *reply = provider == Provider::Cnr ? m_network.post(request, "{}") : m_network.get(request);
    m_reply = reply;
    connect(reply, &QNetworkReply::downloadProgress, this, [reply](qint64 received, qint64) {
        if (received > 2 * 1024 * 1024)
            reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId, ticket, provider, channel] {
        reply->deleteLater();
        if (requestId != m_request)
            return;
        m_reply = nullptr;
        if (reply->error() != QNetworkReply::NoError) {
            emit failed(ticket, QStringLiteral("无法连接电台目录，请检查网络后重试"));
            return;
        }
        const auto bytes = reply->readAll();
        if (bytes.size() > 2 * 1024 * 1024) {
            emit failed(ticket, QStringLiteral("电台目录响应异常"));
            return;
        }
        QUrl stream;
        if (provider == Provider::Cnr) {
            const auto document = QJsonDocument::fromJson(bytes);
            const auto categories = document.object()["data"].toObject()["categories"].toArray();
            if (!categories.isEmpty())
                for (const auto &value : categories.first().toObject()["detail"].toArray()) {
                    const auto entry = value.toObject();
                    if (entry["id"].toString() != channel)
                        continue;
                    for (const auto &candidate : entry["other_info11"].toArray()) {
                        const QUrl url(candidate.toObject()["url"].toString());
                        if (url.scheme() == "https" && url.host().endsWith(".cnr.cn")) {
                            stream = url;
                            break;
                        }
                    }
                }
        } else {
            const auto match = QRegularExpression("https://rthkradio1-live[^\\s\"<>]+\\.m3u8")
                                   .match(QString::fromUtf8(bytes));
            if (match.hasMatch())
                stream = QUrl(match.captured());
        }
        if (!stream.isValid() || stream.scheme() != "https") {
            emit failed(ticket, QStringLiteral("电台暂未提供可用直播地址"));
            return;
        }
        emit resolved(ticket, stream);
    });
}
