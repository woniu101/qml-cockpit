#pragma once

#include "AssistantInference.h"

#include <QAudioFormat>
#include <QMediaDevices>
#include <QTimer>
#include <memory>

class QAudioSource;
class QIODevice;
class QUrl;
class AssistantAudioRoute;

class AssistantWake final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool configured READ configured CONSTANT)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY changed)
    Q_PROPERTY(bool listening READ listening NOTIFY changed)
    Q_PROPERTY(bool bargeIn READ bargeIn WRITE setBargeIn NOTIFY changed)
    Q_PROPERTY(bool uiBlocked READ uiBlocked WRITE setUiBlocked NOTIFY changed)
    Q_PROPERTY(bool testing READ testing NOTIFY changed)
    Q_PROPERTY(QString keyword READ keyword NOTIFY changed)
    Q_PROPERTY(QString defaultKeyword READ defaultKeyword CONSTANT)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString testResult READ testResult NOTIFY changed)
    Q_PROPERTY(Feedback feedback READ feedback WRITE setFeedback NOTIFY feedbackChanged)
    Q_PROPERTY(bool preparing READ preparing NOTIFY changed)
public:
    enum Feedback { Tone, Spoken, Silent };
    Q_ENUM(Feedback)
    AssistantWake();
    ~AssistantWake() override;
    bool configured() const { return m_worker.configured(); }
    bool enabled() const { return m_enabled; }
    bool listening() const { return m_input != nullptr; }
    bool bargeIn() const { return m_bargeIn; }
    bool uiBlocked() const { return m_uiBlocked; }
    void setUiBlocked(bool value) {
        if (m_uiBlocked == value) return;
        m_uiBlocked = value;
        if (value) setSuspended(true);
        emit changed();
    }
    void setBargeIn(bool value);
    bool testing() const { return m_queued != Operation::None || m_job == Operation::Configure || m_job == Operation::Replay; }
    QString keyword() const { return m_keyword; }
    static QString defaultKeyword() { return QStringLiteral("你好周同学"); }
    QString message() const { return m_message; }
    QString testResult() const { return m_testResult; }
    Feedback feedback() const { return m_feedback; }
    void setFeedback(Feedback value);
    bool preparing() const { return m_preparing; }
    void setPreparing(bool value);
    void setEnabled(bool value);
    void setSuspended(bool value);
    void setInputRoute(AssistantAudioRoute *route);
    Q_INVOKABLE void saveKeyword(const QString &word);
    Q_INVOKABLE void replay(const QUrl &url);

signals:
    void changed();
    void activated();
    void feedbackChanged();

private:
    enum class Operation { None, Prepare, Configure, Replay, Feed };
    void enqueue(Operation operation, const QJsonObject &request);
    void drive();
    void complete(const QJsonObject &result);
    void startCapture();
    void stopCapture();
    void captureFailed(const QString &message);
    void showStatus(const QString &message);
    void resetInput();

    AssistantInference m_worker;
    QMediaDevices m_devices;
    AssistantAudioRoute *m_route = nullptr;
    QTimer m_resume;
    std::unique_ptr<QAudioSource> m_capture;
    QIODevice *m_input = nullptr;
    QAudioFormat m_format;
    QByteArray m_audio;
    QJsonObject m_request;
    QString m_keyword = defaultKeyword();
    QString m_nextKeyword, m_readyKeyword, m_message, m_testResult;
    Operation m_job = Operation::None, m_queued = Operation::None;
    int m_stream = 0, m_jobStream = 0;
    bool m_enabled = false, m_suspended = false, m_captureFailed = false, m_bargeIn = false;
    bool m_uiBlocked = false;
    Feedback m_feedback = Tone;
    bool m_preparing = false;
};
