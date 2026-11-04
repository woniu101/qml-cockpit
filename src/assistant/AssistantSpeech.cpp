#include "AssistantSpeech.h"
#include "AssistantAudioInput.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QAudioSource>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMediaDevices>
#include <QUrl>

AssistantSpeech::AssistantSpeech(QAudioOutput &output)
    : m_mediaOutput(output), m_python(qEnvironmentVariable("CAR_TERMINAL_ASR_PYTHON")),
      m_worker(qEnvironmentVariable("CAR_TERMINAL_ASR_WORKER")), m_model(qEnvironmentVariable("CAR_TERMINAL_ASR_MODEL")) {
    m_limit.setSingleShot(true);
    connect(&m_limit, &QTimer::timeout, this, [this] {
        if (recording())
            finish();
        else if (recognizing())
            fail(QStringLiteral("离线识别超时，请重试"));
    });
    connect(&m_asr, &QProcess::started, this, [this] {
        if (!recognizing()) {
            m_asr.kill();
            return;
        }
        m_asr.write(m_pending);
        m_pending.clear();
        m_asr.closeWriteChannel();
    });
    connect(&m_asr, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (recognizing() && error == QProcess::FailedToStart)
            fail(QStringLiteral("无法启动离线语音识别"));
    });
    connect(&m_asr, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        const auto output = m_asr.readAllStandardOutput();
        const auto error = m_asr.readAllStandardError();
        if (!recognizing())
            return;
        const auto text = QJsonDocument::fromJson(output).object().value("text").toString().trimmed();
        if (code != 0 || status != QProcess::NormalExit) {
            qWarning("Offline ASR: %s", error.left(800).constData());
            fail(QStringLiteral("离线识别失败，请检查语音运行环境"));
        } else if (text.isEmpty()) {
            fail(QStringLiteral("没有听清，请再说一次"));
        } else {
            m_limit.stop();
            m_state = State::Idle;
            m_message = text;
            emit changed();
            emit recognized(text);
        }
    });
}

AssistantSpeech::~AssistantSpeech() {
    cancel();
    if (m_asr.state() != QProcess::NotRunning) {
        m_asr.kill();
        m_asr.waitForFinished(1000);
    }
}

bool AssistantSpeech::configured() const {
    return QFileInfo(m_python).isFile() && QFileInfo(m_worker).isFile() &&
        QFileInfo(m_model + "/model.int8.onnx").isFile() && QFileInfo(m_model + "/tokens.txt").isFile();
}

void AssistantSpeech::start() {
    if (m_state != State::Idle || m_asr.state() != QProcess::NotRunning)
        return;
    if (!configured()) {
        fail(QStringLiteral("请通过助手开发启动脚本加载离线语音模型"));
        return;
    }
    const auto device = QMediaDevices::defaultAudioInput();
    if (device.isNull()) {
        fail(QStringLiteral("没有找到可用的麦克风"));
        return;
    }
    const auto format = assistantInputFormat(device);
    const auto encoding = assistantInputEncoding(format);
    if (!format.isValid() || encoding.isEmpty()) {
        fail(QStringLiteral("麦克风不支持可识别的录音格式"));
        return;
    }
    emit captureRequested();
    m_buffer.setData({});
    m_buffer.open(QIODevice::WriteOnly);
    m_format = {encoding, QString::number(format.sampleRate()), QString::number(format.channelCount())};
    m_source = std::make_unique<QAudioSource>(device, format);
    m_state = State::Recording;
    updateAudioFocus();
    connect(m_source.get(), &QAudioSource::stateChanged, this, [this] {
        if (recording() && m_source && m_source->state() == QtAudio::StoppedState && m_source->error() != QtAudio::NoError)
            fail(QStringLiteral("无法录音，请检查麦克风权限或设备连接"));
    }, Qt::QueuedConnection);
    m_source->start(&m_buffer);
    m_message = QStringLiteral("正在听，点击结束录音");
    m_limit.start(8000);
    emit changed();
}

void AssistantSpeech::stopCapture() {
    if (m_source) {
        m_source->stop();
        m_source.reset();
    }
    m_buffer.close();
    updateAudioFocus();
}

void AssistantSpeech::updateAudioFocus() {
    const bool quiet = recording() || m_replyPlaying;
    if (quiet && !m_mediaOutput.isMuted()) {
        m_restoreMute = true;
        m_mediaOutput.setMuted(true);
    } else if (!quiet && m_restoreMute) {
        m_mediaOutput.setMuted(false);
        m_restoreMute = false;
    }
}

void AssistantSpeech::finish() {
    if (!recording())
        return;
    m_state = State::Idle;
    stopCapture();
    auto audio = m_buffer.data();
    m_buffer.setData({});
    recognize(std::move(audio), m_format);
}

void AssistantSpeech::openWave(const QUrl &url) {
    if (m_state != State::Idle || m_asr.state() != QProcess::NotRunning)
        return;
    const QFileInfo info(url.toLocalFile());
    constexpr qint64 maxBytes = 32 * 1024 * 1024;
    if (!url.isLocalFile() || !info.isFile() || info.suffix().compare("wav", Qt::CaseInsensitive) != 0 ||
        info.size() <= 0 || info.size() > maxBytes) {
        fail(QStringLiteral("请选择不超过 32 MB 的本地 WAV 文件"));
        return;
    }
    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        fail(QStringLiteral("无法读取所选 WAV 文件"));
        return;
    }
    recognize(file.read(maxBytes + 1), {"wav"});
}

void AssistantSpeech::recognize(QByteArray audio, const QStringList &format) {
    if (m_state != State::Idle || m_asr.state() != QProcess::NotRunning)
        return;
    if (!configured() || audio.isEmpty() || audio.size() > 32 * 1024 * 1024) {
        fail(QStringLiteral("录音不可用或离线识别未配置"));
        return;
    }
    m_pending = std::move(audio);
    m_state = State::Recognizing;
    m_message = QStringLiteral("正在离线识别…");
    m_limit.start(15000);
    m_asr.start(m_python, QStringList{"-I", "-B", m_worker, m_model} + format);
    emit changed();
}

void AssistantSpeech::cancel() {
    if (m_state != State::Idle)
        fail(QStringLiteral("已取消录音"));
}

void AssistantSpeech::fail(const QString &message) {
    m_state = State::Idle;
    m_limit.stop();
    stopCapture();
    m_pending.clear();
    m_buffer.setData({});
    if (m_asr.state() != QProcess::NotRunning)
        m_asr.kill();
    m_message = message;
    emit changed();
}

void AssistantSpeech::setReplyPlaying(bool playing) {
    m_replyPlaying = playing;
    updateAudioFocus();
}
