#pragma once

#include <QBuffer>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <memory>

class QAudioOutput;
class QAudioSource;
class QUrl;

class AssistantSpeech final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool configured READ configured CONSTANT)
    Q_PROPERTY(bool recording READ recording NOTIFY changed)
    Q_PROPERTY(bool recognizing READ recognizing NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
public:
    explicit AssistantSpeech(QAudioOutput &);
    ~AssistantSpeech() override;
    bool configured() const;
    bool recording() const { return m_state == State::Recording; }
    bool recognizing() const { return m_state == State::Recognizing; }
    QString message() const { return m_message; }
    Q_INVOKABLE void start();
    Q_INVOKABLE void finish();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void openWave(const QUrl &url);
    void setReplyPlaying(bool playing);

signals:
    void captureRequested();
    void changed();
    void recognized(const QString &text);

private:
    enum class State { Idle, Recording, Recognizing };
    void recognize(QByteArray audio, const QStringList &format);
    void stopCapture();
    void updateAudioFocus();
    void fail(const QString &message);
    QAudioOutput &m_mediaOutput;
    QBuffer m_buffer;
    std::unique_ptr<QAudioSource> m_source;
    QProcess m_asr;
    QTimer m_limit;
    QByteArray m_pending;
    QString m_python, m_worker, m_model, m_message;
    QStringList m_format;
    State m_state = State::Idle;
    bool m_restoreMute = false;
    bool m_replyPlaying = false;
};
