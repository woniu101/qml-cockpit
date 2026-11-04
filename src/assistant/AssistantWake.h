#pragma once

#include "AssistantWorker.h"

#include <QAudioFormat>
#include <QMediaDevices>
#include <memory>

class QAudioSource;
class QIODevice;
class QUrl;

class AssistantWake final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool configured READ configured CONSTANT)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY changed)
    Q_PROPERTY(bool listening READ listening NOTIFY changed)
    Q_PROPERTY(bool testing READ testing NOTIFY changed)
    Q_PROPERTY(QString keyword READ keyword NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString testResult READ testResult NOTIFY changed)
public:
    AssistantWake();
    ~AssistantWake() override;
    bool configured() const { return m_worker.configured(); }
    bool enabled() const { return m_enabled; }
    bool listening() const { return m_input != nullptr; }
    bool testing() const { return m_queued != Operation::None || m_job == Operation::Configure || m_job == Operation::Replay; }
    QString keyword() const { return m_keyword; }
    QString message() const { return m_message; }
    QString testResult() const { return m_testResult; }
    void setEnabled(bool value);
    void setSuspended(bool value);
    Q_INVOKABLE void saveKeyword(const QString &word);
    Q_INVOKABLE void replay(const QUrl &url);

signals:
    void changed();
    void activated();

private:
    enum class Operation { None, Prepare, Configure, Replay, Feed };
    void enqueue(Operation operation, const QJsonObject &request);
    void drive();
    void complete(const QJsonObject &result);
    void startCapture();
    void stopCapture();
    void captureFailed(const QString &message);
    void showStatus(const QString &message);

    AssistantWorker m_worker;
    QMediaDevices m_devices;
    QTimer m_resume;
    std::unique_ptr<QAudioSource> m_capture;
    QIODevice *m_input = nullptr;
    QAudioFormat m_format;
    QByteArray m_audio;
    QJsonObject m_request;
    QString m_keyword = QStringLiteral("你好小林");
    QString m_nextKeyword, m_readyKeyword, m_message, m_testResult;
    Operation m_job = Operation::None, m_queued = Operation::None;
    int m_stream = 0, m_jobStream = 0;
    bool m_enabled = false, m_suspended = false, m_captureFailed = false;
};
