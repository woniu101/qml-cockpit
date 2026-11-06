#pragma once

#include <QAudioDecoder>
#include <QAudioFormat>
#include <QBuffer>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>
#include <memory>

class QAudioDevice;
class QAudioSource;

class AssistantAudioClip final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool ready READ ready NOTIFY changed)
    Q_PROPERTY(bool recording READ recording NOTIFY changed)
    Q_PROPERTY(double duration READ duration NOTIFY changed)
    Q_PROPERTY(double start READ start NOTIFY changed)
    Q_PROPERTY(double length READ length NOTIFY changed)
    Q_PROPERTY(double level READ level NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString name READ name NOTIFY changed)
    Q_PROPERTY(QUrl wave READ wave NOTIFY changed)
public:
    AssistantAudioClip(double sourceLimit = 120, double minimum = 1, double clipLimit = 15);
    ~AssistantAudioClip() override;
    bool busy() const { return m_busy; }
    bool ready() const { return !m_wave.isEmpty(); }
    bool recording() const { return bool(m_capture); }
    double duration() const { return m_pcm.size() / 32000.0; }
    double start() const { return m_start; }
    double length() const { return m_length; }
    double level() const { return m_level; }
    QString message() const { return m_message; }
    QString name() const { return m_name; }
    QUrl wave() const;
    Q_INVOKABLE void load(const QUrl &url);
    Q_INVOKABLE bool select(double start, double length);
    Q_INVOKABLE void cancel();
    void startRecording(const QAudioDevice &device);
    Q_INVOKABLE void finishRecording();

signals:
    void changed();
    void loaded();
    void failed(const QString &message);

private:
    void append(const QByteArray &bytes, const QAudioFormat &format);
    void finish();
    void fail(const QString &message);
    void stop();
    const double m_sourceLimit, m_minimum, m_clipLimit;
    QTemporaryDir m_directory;
    std::unique_ptr<QAudioDecoder> m_decoder;
    std::unique_ptr<QAudioSource> m_capture;
    QBuffer m_recorded;
    QAudioFormat m_format;
    QTimer m_timeout, m_meter;
    QByteArray m_pcm;
    QString m_wave, m_name, m_message;
    double m_start = 0, m_length = 0, m_level = 0;
    qsizetype m_offset = 0;
    bool m_busy = false;
    int m_revision = 0;
    QString m_decodeError;
};
