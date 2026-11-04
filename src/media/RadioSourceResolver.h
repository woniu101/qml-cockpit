#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QNetworkReply>

class RadioSourceResolver : public QObject {
    Q_OBJECT
  public:
    enum class Provider { Cnr, Rthk };
    explicit RadioSourceResolver(QObject *parent = nullptr) : QObject(parent) {}
    void resolve(Provider, const QString &channel, quint64 ticket);
    void cancel();
  signals:
    void resolved(quint64, const QUrl &);
    void failed(quint64, const QString &);

  private:
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    quint64 m_request = 0;
};
