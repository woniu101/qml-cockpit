#include "AssistantWake.h"
#include "AssistantAudioRoute.h"
#include "AssistantAudioInput.h"

#include <QAudioSource>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QSettings>
#include <QUrl>

AssistantWake::AssistantWake()
    : m_worker(cockpit::InferenceKind::Wake, cockpit::modelDirectory(cockpit::InferenceKind::Wake)) {
    if (!qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS")) {
        const QSettings settings;
        m_keyword = settings.value("assistant/wakeKeyword", m_keyword).toString();
        m_enabled = settings.value("assistant/wakeEnabled", false).toBool();
        m_bargeIn = settings.value("assistant/wakeBargeIn", false).toBool();
        const auto feedback = settings.value("assistant/wakeFeedback").toString();
        if (feedback == "spoken")
            m_feedback = Spoken;
        else if (feedback == "silent")
            m_feedback = Silent;
    }
    m_message = m_enabled ? QStringLiteral("正在准备离线唤醒") : QStringLiteral("当前使用按键说话");
    m_resume.setSingleShot(true);
    connect(&m_resume, &QTimer::timeout, this, &AssistantWake::drive);
    connect(&m_worker, &AssistantInference::completed, this, &AssistantWake::complete);
    connect(&m_devices, &QMediaDevices::audioInputsChanged, this, [this] { if (!m_route) resetInput(); });
    if (m_enabled)
        m_resume.start(0);
}

AssistantWake::~AssistantWake() { stopCapture(); }

void AssistantWake::setFeedback(Feedback value) {
    if (value == m_feedback || value < Tone || value > Silent)
        return;
    m_feedback = value;
    if (!qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS"))
        QSettings().setValue("assistant/wakeFeedback", value == Spoken ? "spoken" : value == Silent ? "silent" : "tone");
    emit feedbackChanged();
}

void AssistantWake::setPreparing(bool value) {
    if (value == m_preparing)
        return;
    m_preparing = value;
    emit changed();
}

void AssistantWake::setInputRoute(AssistantAudioRoute *route) {
    m_route = route;
    connect(route, &AssistantAudioRoute::deviceChanged, this, &AssistantWake::resetInput);
    resetInput();
}

void AssistantWake::resetInput() {
    if (!m_enabled)
        return;
    stopCapture();
    m_captureFailed = false;
    drive();
}

void AssistantWake::setBargeIn(bool value) {
    if (m_bargeIn == value)
        return;
    m_bargeIn = value;
    if (!qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS"))
        QSettings().setValue("assistant/wakeBargeIn", value);
    emit changed();
}

void AssistantWake::setEnabled(bool value) {
    if (value == m_enabled)
        return;
    if (value && !configured()) {
        showStatus(QStringLiteral("请通过助手开发启动脚本加载唤醒模型"));
        return;
    }
    m_enabled = value;
    if (!qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS"))
        QSettings().setValue("assistant/wakeEnabled", value);
    m_captureFailed = false;
    m_resume.stop();
    if (!value) {
        stopCapture();
        showStatus(QStringLiteral("当前使用按键说话"));
    } else {
        showStatus(QStringLiteral("正在准备离线唤醒"));
        drive();
    }
}

void AssistantWake::setSuspended(bool value) {
    if (value == m_suspended)
        return;
    m_suspended = value;
    if (value) {
        m_resume.stop();
        if (listening()) {
            stopCapture();
            showStatus(QStringLiteral("指令处理期间暂停唤醒"));
        }
    } else if (m_enabled) {
        m_resume.start(750);
    }
}

void AssistantWake::saveKeyword(const QString &word) {
    if (!configured() || testing())
        return;
    m_nextKeyword = word.trimmed();
    enqueue(Operation::Configure, {{"operation", "configure"}, {"keyword", m_nextKeyword}});
}

void AssistantWake::replay(const QUrl &url) {
    if (!configured() || testing())
        return;
    constexpr qint64 limit = 4 * 1024 * 1024;
    const QFileInfo info(url.toLocalFile());
    QFile input(info.absoluteFilePath());
    if (!url.isLocalFile() || !info.isFile() || info.suffix().compare("wav", Qt::CaseInsensitive) != 0 ||
        info.size() <= 0 || info.size() > limit || !input.open(QIODevice::ReadOnly)) {
        m_testResult = QStringLiteral("请选择不超过 4 MB 的本地 PCM16 WAV");
        emit changed();
        return;
    }
    const auto data = input.read(limit + 1);
    if (data.size() > limit) {
        m_testResult = QStringLiteral("WAV 文件超出试测大小限制");
        emit changed();
        return;
    }
    enqueue(Operation::Replay, {{"operation", "replay"}, {"keyword", m_keyword},
                                {"audio", QString::fromLatin1(data.toBase64())}});
}

void AssistantWake::enqueue(Operation operation, const QJsonObject &request) {
    stopCapture();
    m_queued = operation;
    m_request = request;
    m_testResult = operation == Operation::Replay ? QStringLiteral("正在离线试测 WAV…") : QStringLiteral("正在验证唤醒词…");
    emit changed();
    drive();
}

void AssistantWake::drive() {
    if (m_job != Operation::None)
        return;
    if (m_queued != Operation::None) {
        if (m_worker.submit(m_request)) {
            m_job = m_queued;
            m_queued = Operation::None;
            m_request = {};
        }
        return;
    }
    if (!m_enabled || m_suspended || m_resume.isActive() || m_captureFailed)
        return;
    if (m_readyKeyword != m_keyword) {
        if (m_worker.submit({{"operation", "configure"}, {"keyword", m_keyword}}))
            m_job = Operation::Prepare;
        else if (!configured())
            captureFailed(QStringLiteral("唤醒模型未配置，可继续按键说话"));
        return;
    }
    if (!listening())
        startCapture();
    if (!listening())
        return;
    const int bytes = m_format.bytesForFrames(m_format.sampleRate() / 5);
    if (m_audio.size() < bytes)
        return;
    if (m_worker.submit({{"operation", "feed"}, {"keyword", m_keyword}, {"stream", m_stream},
                         {"encoding", assistantInputEncoding(m_format)}, {"rate", m_format.sampleRate()},
                         {"channels", m_format.channelCount()}, {"audio", QString::fromLatin1(m_audio.first(bytes).toBase64())}})) {
        m_job = Operation::Feed;
        m_jobStream = m_stream;
        m_audio.remove(0, bytes);
    }
}

void AssistantWake::complete(const QJsonObject &result) {
    const auto job = m_job;
    m_job = Operation::None;
    const bool ok = result.value("ok").toBool();
    const auto error = result.value("error").toString();
    if (job == Operation::Prepare) {
        if (ok)
            m_readyKeyword = m_keyword;
        else if (m_enabled)
            captureFailed(QStringLiteral("离线唤醒准备失败：%1").arg(error));
    } else if (job == Operation::Configure) {
        if (ok) {
            m_keyword = m_nextKeyword;
            m_readyKeyword = m_keyword;
            if (!qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS"))
                QSettings().setValue("assistant/wakeKeyword", m_keyword);
        }
        m_testResult = ok ? QStringLiteral("唤醒词已保存") : error;
        emit changed();
    } else if (job == Operation::Replay) {
        m_testResult = !ok ? error : result.value("hit").toBool()
            ? QStringLiteral("WAV 命中「%1」").arg(m_keyword)
            : QStringLiteral("WAV 未命中「%1」").arg(m_keyword);
        emit changed();
    } else if (job == Operation::Feed && m_jobStream == m_stream && listening() && m_enabled && !m_suspended) {
        if (!ok) {
            captureFailed(QStringLiteral("离线唤醒失败：%1").arg(error));
        } else if (result.value("hit").toBool()) {
            stopCapture();
            m_suspended = true;
            showStatus(QStringLiteral("已唤醒，开始收听指令"));
            emit activated();
        }
    }
    drive();
}

void AssistantWake::startCapture() {
    if (!configured()) {
        captureFailed(QStringLiteral("唤醒模型未配置，可继续按键说话"));
        return;
    }
    const auto device = m_route ? m_route->device() : QMediaDevices::defaultAudioInput();
    if (device.isNull()) {
        captureFailed(QStringLiteral("找不到麦克风，连接后会重新尝试"));
        return;
    }
    m_format = assistantInputFormat(device);
    if (!m_format.isValid() || assistantInputEncoding(m_format).isEmpty() ||
        m_format.sampleRate() < 8000 || m_format.sampleRate() > 192000 || m_format.channelCount() > 8) {
        captureFailed(QStringLiteral("麦克风格式不支持离线唤醒"));
        return;
    }
    m_capture = std::make_unique<QAudioSource>(device, m_format);
    connect(m_capture.get(), &QAudioSource::stateChanged, this, [this] {
        if (m_capture && m_capture->state() == QtAudio::StoppedState && m_capture->error() != QtAudio::NoError)
            captureFailed(QStringLiteral("无法监听，请检查麦克风连接或权限"));
    }, Qt::QueuedConnection);
    m_input = m_capture->start();
    if (!m_input) {
        captureFailed(QStringLiteral("无法启动麦克风监听"));
        return;
    }
    connect(m_input, &QIODevice::readyRead, this, [this] {
        if (!m_input)
            return;
        m_audio += m_input->readAll();
        const int limit = m_format.bytesForFrames(m_format.sampleRate());
        const int frame = m_format.bytesPerFrame();
        if (m_audio.size() > limit) {
            m_audio.remove(0, ((m_audio.size() - limit) / frame) * frame);
            ++m_stream;
        }
        drive();
    });
    showStatus(QStringLiteral("正在本机监听「%1」").arg(m_keyword));
}

void AssistantWake::stopCapture() {
    ++m_stream;
    if (m_input)
        disconnect(m_input, nullptr, this, nullptr);
    m_input = nullptr;
    if (m_capture)
        m_capture->stop();
    m_capture.reset();
    m_audio.clear();
}

void AssistantWake::captureFailed(const QString &message) {
    stopCapture();
    m_captureFailed = true;
    showStatus(message);
}

void AssistantWake::showStatus(const QString &message) {
    m_message = message;
    emit changed();
}
