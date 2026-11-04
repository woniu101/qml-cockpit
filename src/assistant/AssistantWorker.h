#pragma once

#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QTimer>

class AssistantWorker final : public QObject {
    Q_OBJECT
public:
    AssistantWorker(QString python, QString script, QString model, int timeoutMs);
    ~AssistantWorker() override;
    bool configured() const;
    bool submit(const QJsonObject &request);

signals:
    void completed(const QJsonObject &result);

private:
    void writeRequest();
    void finish(const QJsonObject &result);
    QProcess m_process;
    QTimer m_timeout;
    QString m_python, m_script, m_model;
    QByteArray m_pending;
    bool m_busy = false;
};
