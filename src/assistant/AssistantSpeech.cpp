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
#include <QJsonArray>
#include <QMediaDevices>
#include <QSettings>
#include <QUrl>
#include <algorithm>
#include <cmath>

AssistantSpeech::AssistantSpeech(QAudioOutput &output)
    : m_mediaOutput(output), m_asr(cockpit::InferenceKind::Asr, cockpit::modelDirectory(cockpit::InferenceKind::Asr)),
      m_model(cockpit::modelDirectory(cockpit::InferenceKind::Asr)) {
    connect(&m_clip, &AssistantAudioClip::changed, this, &AssistantSpeech::changed);
    connect(&m_clip, &AssistantAudioClip::failed, this, &AssistantSpeech::fail);
    connect(&m_clip, &AssistantAudioClip::loaded, this, [this] {
        if (!m_dictating && m_clip.duration() > 10.01) {
            fail(QStringLiteral("指令录音最长 10 秒，请选择较短的片段"));
            return;
        }
        QFile file(m_clip.wave().toLocalFile());
        if (!file.open(QIODevice::ReadOnly)) {
            fail(QStringLiteral("无法读取音频片段"));
            return;
        }
        recognize(file.readAll(), {"wav"});
    });
    connect(&m_route, &AssistantAudioRoute::changed, this, &AssistantSpeech::changed);
    if (!qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS"))
        m_continuous = QSettings().value("assistant/continuous", false).toBool();
    m_endpointTimer.setInterval(50);
    connect(&m_endpointTimer, &QTimer::timeout, this, &AssistantSpeech::checkEndpoint);
    connect(&m_route, &AssistantAudioRoute::deviceChanged, this, [this] {
        if (recording())
            fail(QStringLiteral("麦克风已切换或断开，请重新说一次"));
    });
    m_limit.setSingleShot(true);
    connect(&m_limit, &QTimer::timeout, this, [this] {
        if (recording()) {
            if (m_automatic && !m_endpoint.spoke())
                fail(QStringLiteral("未听到指令，已结束收音"));
            else
                finish();
        }
        else if (recognizing())
            fail(QStringLiteral("离线识别超时，请重试"));
    });
    connect(&m_asr, &AssistantInference::completed, this, [this](const QJsonObject &result) {
        if (m_state != State::Recognizing)
            return;
        const auto text = result.value("text").toString().trimmed();
        if (result.contains("error")) {
            qWarning("Offline ASR: %s", result.value("error").toString().toUtf8().constData());
            fail(QStringLiteral("离线识别失败，请检查语音运行环境"));
        } else if (text.isEmpty()) {
            fail(QStringLiteral("没有听清，请再说一次"));
        } else {
            m_limit.stop();
            m_state = State::Idle;
            m_followUpWanted = m_fromMicrophone && m_continuous && !m_dictating;
            m_message = text;
            m_failed = false;
            emit changed();
            if (m_dictating)
                emit dictated(text, m_dictationTarget);
            else
                emit recognized(text);
        }
    });
}

AssistantSpeech::~AssistantSpeech() {
    cancel();

}

bool AssistantSpeech::configured() const {
    return m_asr.configured() && QFileInfo(m_model + "/model.int8.onnx").isFile() && QFileInfo(m_model + "/tokens.txt").isFile();
}

bool AssistantSpeech::inputAvailable() const {
    return m_route.available();
}

void AssistantSpeech::setContinuous(bool value) {
    if (m_continuous == value)
        return;
    m_continuous = value;
    if (!value)
        m_followUpWanted = false;
    if (!qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS"))
        QSettings().setValue("assistant/continuous", value);
    emit changed();
}

void AssistantSpeech::setSessionOpen(bool open) {
    if (m_sessionOpen == open)
        return;
    m_sessionOpen = open;
    if (!open)
        m_followUpWanted = false;
    emit changed();
}

void AssistantSpeech::start() {
    if (recording() || recognizing()) return;
    m_dictating = false;
    startCapture(m_continuous);
}

void AssistantSpeech::startAutomatic() {
    if (recording() || recognizing()) return;
    m_dictating = false;
    startCapture(true);
}

void AssistantSpeech::dictate(const QString &target) {
    if (recording() || recognizing())
        return;
    m_dictating = true;
    m_dictationTarget = target;
    startCapture(false);
}

void AssistantSpeech::startCapture(bool automatic) {
    if (m_state != State::Idle || m_clip.busy())
        return;
    if (!configured()) {
        fail(QStringLiteral("请通过助手开发启动脚本加载离线语音模型"));
        return;
    }
    const auto device = m_route.device();
    if (device.isNull()) {
        fail(QStringLiteral("所选收音设备不可用，请检查助手声音设置"));
        return;
    }
    const auto format = assistantInputFormat(device);
    const auto encoding = assistantInputEncoding(format);
    if (!format.isValid() || encoding.isEmpty()) {
        fail(QStringLiteral("麦克风不支持可识别的录音格式"));
        return;
    }
    emit captureRequested();
    m_failed = false;
    m_followUpWanted = false;
    m_fromMicrophone = true;
    m_automatic = automatic;
    m_captureFormat = format;
    m_endpoint = {};
    m_endpointOffset = 0;
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
    m_message = automatic ? QStringLiteral("正在听，停顿后自动提交") : QStringLiteral("正在听，点击结束录音");
    m_endpointTimer.start();
    m_limit.start(8000);
    emit changed();
}

void AssistantSpeech::stopCapture() {
    m_endpointTimer.stop();
    if (m_source) {
        m_source->stop();
        m_source.reset();
    }
    m_buffer.close();
    updateAudioFocus();
}

void AssistantSpeech::checkEndpoint() {
    const auto &audio = m_buffer.data();
    const int bytesPerFrame = m_captureFormat.bytesPerFrame();
    const qsizetype chunk = m_captureFormat.bytesForFrames(m_captureFormat.sampleRate() / 20);
    while (bytesPerFrame > 0 && chunk > 0 && audio.size() - m_endpointOffset >= chunk) {
        const auto result = m_endpoint.observe(QByteArrayView(audio).sliced(m_endpointOffset, chunk), m_captureFormat);
        double peak = 0;
        for (qsizetype i = m_endpointOffset; i < m_endpointOffset + chunk; i += m_captureFormat.bytesPerSample())
            peak = std::max(peak, double(std::abs(m_captureFormat.normalizedSampleValue(audio.constData() + i))));
        m_level = std::min(1.0, peak * 2);
        m_endpointOffset += chunk;
        if (m_automatic && result == AssistantEndpoint::Result::Finished) {
            finish();
            return;
        }
        if (m_automatic && result == AssistantEndpoint::Result::Silence) {
            fail(QStringLiteral("未听到指令，已结束收音"));
            return;
        }
    }
    emit changed();
}

void AssistantSpeech::updateAudioFocus() {
    const bool quiet = recording() || m_replyPlaying || m_otherRecording;
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
    if (m_state != State::Idle)
        return;
    m_dictating = false;
    m_fromMicrophone = false;
    m_followUpWanted = false;
    m_clip.load(url);
}

void AssistantSpeech::transcribe(const QUrl &url, const QString &target) {
    if (recording() || recognizing())
        return;
    m_dictating = true;
    m_dictationTarget = target;
    m_fromMicrophone = false;
    m_followUpWanted = false;
    m_clip.load(url);
}

void AssistantSpeech::recognize(QByteArray audio, const QStringList &format) {
    if (m_state != State::Idle)
        return;
    if (!configured() || audio.isEmpty() || audio.size() > 32 * 1024 * 1024) {
        fail(QStringLiteral("录音不可用或离线识别未配置"));
        return;
    }
    m_failed = false;
    m_state = State::Recognizing;
    m_message = QStringLiteral("正在离线识别…");
    m_limit.start(15000);
    QJsonArray formats;
    for (const auto &part : format) formats.append(part);
    if (!m_asr.submit({{"audio", QString::fromLatin1(audio.toBase64())}, {"format", formats}, {"reference", m_dictating}})) {
        fail(QStringLiteral("无法提交原生语音识别请求"));
        return;
    }
    emit changed();
}

void AssistantSpeech::cancel() {
    m_clip.cancel();
    m_followUpWanted = false;
    if (m_state != State::Idle)
        fail({});
    else if (!m_message.isEmpty()) {
        m_message.clear();
        emit changed();
    }
}

void AssistantSpeech::fail(const QString &message) {
    m_failed = !message.isEmpty();
    m_followUpWanted = false;
    m_state = State::Idle;
    m_limit.stop();
    stopCapture();
    m_buffer.setData({});
    m_asr.cancel();
    m_message = message;
    emit changed();
}

void AssistantSpeech::setReplyPlaying(bool playing) {
    m_replyPlaying = playing;
    updateAudioFocus();
}
