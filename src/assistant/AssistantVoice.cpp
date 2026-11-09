#include "AssistantVoice.h"

#include <QAudioDevice>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <algorithm>
#include <utility>

namespace {
constexpr qint64 MaxWav = 6 * 1024 * 1024;
QByteArray readBounded(const QString &path, qint64 maxBytes) {
    const QFileInfo info(path);
    QFile file(path);
    if (!info.isFile() || info.size() <= 0 || info.size() > maxBytes || !file.open(QIODevice::ReadOnly))
        return {};
    auto data = file.read(maxBytes + 1);
    return data.size() <= maxBytes ? data : QByteArray();
}

bool writeFile(const QString &path, const QByteArray &bytes) {
    QSaveFile file(path);
    return !bytes.isEmpty() && file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
}
}

AssistantVoice::AssistantVoice(QString storage)
    : m_worker(cockpit::InferenceKind::Voice, cockpit::modelDirectory(cockpit::InferenceKind::Voice)) {
    m_storage = std::move(storage);
    m_player.setAudioOutput(&m_output);
    m_countdownTimer.setInterval(1000);
    connect(&m_countdownTimer, &QTimer::timeout, this, [this] {
        --m_countdown;
        if (m_countdown == 0) {
            m_countdownTimer.stop();
            m_clip.startRecording(m_inputRoute ? m_inputRoute->device() : QAudioDevice{});
        }
        emit changed();
    });
    connect(&m_clip, &AssistantAudioClip::changed, this, &AssistantVoice::changed);
    connect(&m_clip, &AssistantAudioClip::failed, this, [this](const QString &message) {
        m_failed = true;
        status(message);
    });
    connect(&m_clip, &AssistantAudioClip::loaded, this, [this] {
        m_draft = m_clip.wave().toLocalFile();
        m_draftName = m_clip.name();
        m_draftTranscript = m_reading ? readingPrompt() : QString();
        m_verifiedTranscript.clear();
        m_dirty = true;
        status(QStringLiteral("音频已准备好，请核对原文"));
        emit transcriptionRequested(m_clip.wave());
    });
    connect(&m_route, &AssistantAudioRoute::deviceChanged, this, [this] {
        if (playing() || synthesizing()) {
            stop();
            status(QStringLiteral("播报设备已切换或断开"));
        }
    });
    if (!qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS")) {
        QSettings settings;
        m_volume = std::clamp(settings.value("assistant/volume", 85).toInt(), 0, 100);
        m_speed = settings.value("assistant/speed", 100).toInt();
        if (m_speed != 85 && m_speed != 115)
            m_speed = 100;
        if (!settings.contains("assistant/replyMode")) {
            settings.setValue("assistant/replyMode", settings.value("assistant/replies", true).toBool() ? "all" : "text");
            settings.remove("assistant/replies");
        }
        m_replyMode = settings.value("assistant/replyMode", "all").toString();
        if (!QStringList{"all", "voice", "text"}.contains(m_replyMode))
            m_replyMode = "all";
    }
    if (!rateAvailable())
        m_speed = 100;
    applyPlaybackPreferences();
    connect(&m_player, &QMediaPlayer::playbackStateChanged, this, &AssistantVoice::changed);
    connect(&m_player, &QMediaPlayer::errorOccurred, this, [this] {
        m_failed = true;
        if (m_preview) m_verifiedTranscript.clear();
        status(QStringLiteral("无法播放语音，请检查播报设备"));
    });
    connect(&m_worker, &AssistantInference::completed, this, &AssistantVoice::complete);
    if (m_storage.isEmpty() && !qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS"))
        m_storage = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/assistant/voice.json";
    const auto saved = QJsonDocument::fromJson(readBounded(m_storage, 2 * MaxWav)).object();
    const auto savedText = saved.value("transcript").toString().trimmed();
    if (m_dir.isValid() && !savedText.isEmpty() &&
        writeFile(m_dir.filePath("voice.wav"), QByteArray::fromBase64(saved.value("wave").toString().toLatin1()))) {
        m_reference = m_dir.filePath("voice.wav");
        m_referenceText = savedText;
        m_draftName = saved.value("name").toString();
    } else {
        m_reference = qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS") ? qEnvironmentVariable("CAR_TERMINAL_TTS_REFERENCE") : cockpit::modelResource("defaultReference");
        const auto transcript = qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS") ? qEnvironmentVariable("CAR_TERMINAL_TTS_TRANSCRIPT") : cockpit::modelResource("defaultTranscript");
        m_referenceText = QString::fromUtf8(readBounded(transcript, 2048)).trimmed();
        if (m_referenceText.startsWith(QChar(0xfeff)))
            m_referenceText.remove(0, 1);
        if (!m_reference.isEmpty())
            m_draftName = QStringLiteral("默认音色");
    }
    m_draft = m_reference;
    m_activeName = m_draftName;
    m_draftTranscript = m_referenceText;
    if (!configured())
        m_message = QStringLiteral("语音功能暂不可用");
}

void AssistantVoice::importReference(const QUrl &url) {
    if (m_busy || m_clip.busy())
        return;
    stop();
    m_failed = false;
    m_reading = false;
    m_clip.load(url);
}

void AssistantVoice::setInputRoute(AssistantAudioRoute *route) {
    m_inputRoute = route;
    connect(route, &AssistantAudioRoute::deviceChanged, this, [this] {
        if (m_countdown > 0 || m_clip.recording()) {
            cancelRecording();
            status(QStringLiteral("收音设备已变化，请重新录制"));
        }
    });
}

void AssistantVoice::setTranscript(const QString &text) {
    if (m_busy || m_draftTranscript == text)
        return;
    m_draftTranscript = text.left(300);
    m_verifiedTranscript.clear();
    m_dirty = true;
    emit changed();
}

void AssistantVoice::setPreviewText(const QString &text) {
    m_previewText = text.left(200);
    emit changed();
}

void AssistantVoice::preview() {
    if (m_clip.busy() || m_previewText.trimmed().isEmpty())
        return;
    m_verifiedTranscript.clear();
    synthesize(m_previewText, m_draft, m_draftTranscript.trimmed(), true);
}

void AssistantVoice::recordReference() {
    if (m_busy || m_clip.busy() || m_countdown > 0)
        return;
    stop();
    m_reading = true;
    m_countdown = 3;
    m_countdownTimer.start();
    status(QStringLiteral("准备朗读提示文本"));
}

void AssistantVoice::cancelRecording() {
    m_countdownTimer.stop();
    m_countdown = 0;
    if (m_clip.recording())
        m_clip.cancel();
    emit changed();
}

void AssistantVoice::playReference(bool active) {
    if (m_busy || m_clip.busy())
        return;
    stop();
    m_failed = false;
    if (!m_route.available()) {
        m_failed = true;
        status(QStringLiteral("播报设备不可用，请检查连接"));
        return;
    }
    m_output.setDevice(m_route.device());
    m_player.setSource(QUrl::fromLocalFile(active ? m_reference : m_draft));
    m_preview = true;
    m_output.setVolume(m_volume / 100.0f);
    m_player.setPlaybackRate(1.0);
    m_player.play();
    status(QStringLiteral("正在播放参考原声"));
}

void AssistantVoice::trimReference(double start, double length) {
    if (m_busy || m_clip.busy())
        return;
    stop();
    if (m_clip.select(start, length)) {
        m_draft = m_clip.wave().toLocalFile();
        m_verifiedTranscript.clear();
        m_draftTranscript.clear();
        m_dirty = true;
        emit transcriptionRequested(m_clip.wave());
        emit changed();
    }
}

void AssistantVoice::discardDraft() {
    if (m_busy)
        return;
    cancelRecording();
    stop();
    m_clip.cancel();
    m_draft = m_reference;
    m_draftName = m_activeName;
    m_draftTranscript = m_referenceText;
    m_verifiedTranscript.clear();
    m_dirty = false;
    m_failed = false;
    status(QStringLiteral("已恢复当前启用的音色"));
}

bool AssistantVoice::save(const QString &transcript) {
    if (m_busy || !verified() || transcript.trimmed() != m_verifiedTranscript)
        return false;
    const auto wav = readBounded(m_draft, MaxWav);
    const QJsonObject profile{{"name", m_draftName}, {"transcript", m_verifiedTranscript},
                              {"wave", QString::fromLatin1(wav.toBase64())}};
    const auto activeReference = m_dir.filePath(QStringLiteral("voice-active-%1.wav").arg(++m_serial));
    if (m_storage.isEmpty() || wav.isEmpty() || !writeFile(activeReference, wav) ||
        !QDir().mkpath(QFileInfo(m_storage).absolutePath()) ||
        !writeFile(m_storage, QJsonDocument(profile).toJson(QJsonDocument::Compact))) {
        m_failed = true;
        status(QStringLiteral("音色保存失败，请检查本地存储"));
        return false;
    }
    m_reference = activeReference;
    m_referenceText = m_verifiedTranscript;
    m_activeName = m_draftName;
    m_dirty = false;
    cancelWakePromptPreparation();
    m_wakePromptAttempted = false;
    m_wakePrompt = QUrl{};
    emit wakePromptChanged();
    prepareWakePrompt();
    status(QStringLiteral("音色已保存，并用于助手回复"));
    return true;
}

void AssistantVoice::stop() {
    const bool busy = m_busy;
    if (busy)
        m_worker.cancel();
    m_busy = false;
    m_player.stop();
    if (busy)
        status(QStringLiteral("已停止本次播报"));
    if (busy)
        prepareWakePrompt();
}

void AssistantVoice::setWakePromptEnabled(bool enabled) {
    if (m_wakePromptEnabled == enabled)
        return;
    m_wakePromptEnabled = enabled;
    if (enabled) {
        m_wakePromptAttempted = !m_wakePrompt.isEmpty();
        prepareWakePrompt();
    } else {
        cancelWakePromptPreparation();
    }
}

void AssistantVoice::prepareWakePrompt() {
    if (!m_wakePromptEnabled || m_wakePromptAttempted || m_busy || !configured() ||
        !QFileInfo(m_reference).isFile() || m_referenceText.isEmpty() || m_referenceText.size() > 300)
        return;
    m_wakePromptFile = m_dir.filePath(QStringLiteral("wake-%1.wav").arg(++m_serial));
    m_wakePromptPreparing = m_worker.submit({{"text", QStringLiteral("我在。")}, {"reference", m_reference},
                                             {"transcript", m_referenceText}, {"output", m_wakePromptFile},
                                             {"wakePrompt", true}});
    m_wakePromptAttempted = m_wakePromptPreparing;
}

void AssistantVoice::cancelWakePromptPreparation() {
    if (!m_wakePromptPreparing)
        return;
    m_worker.cancel();
    m_wakePromptPreparing = false;
    m_wakePromptAttempted = false;
}

void AssistantVoice::reply(const QString &text, bool voiceInput, bool announce) {
    if (announce && (m_replyMode == "all" || (m_replyMode == "voice" && voiceInput)))
        speak(text);
}

void AssistantVoice::speak(const QString &text) {
    m_failed = false;
    if (!configured() || m_reference.isEmpty()) {
        m_failed = true;
        status(QStringLiteral("语音回复未就绪，请在声音设置中检查音色"));
        return;
    }
    if (!m_route.available()) {
        m_failed = true;
        status(QStringLiteral("播报设备不可用，可查看文字回复"));
        return;
    }
    if (m_busy && m_preview) return;
    if (m_busy || playing()) stop();
    synthesize(text, m_reference, m_referenceText, false);
}

bool AssistantVoice::rateAvailable() const {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    return m_player.pitchCompensationAvailability() != QMediaPlayer::PitchCompensationAvailability::Unavailable;
#else
    return false;
#endif
}

void AssistantVoice::applyPlaybackPreferences() {
    m_output.setVolume(m_volume / 100.0f);
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    if (rateAvailable()) {
        m_player.setPitchCompensation(true);
        m_player.setPlaybackRate(m_speed / 100.0);
    }
#endif
}

void AssistantVoice::setVolume(int value) {
    m_volume = std::clamp(value, 0, 100);
    if (!qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS"))
        QSettings().setValue("assistant/volume", m_volume);
    applyPlaybackPreferences();
    emit preferencesChanged();
}

void AssistantVoice::setSpeed(int value) {
    if (!rateAvailable() || (value != 85 && value != 100 && value != 115))
        return;
    m_speed = value;
    if (!qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS"))
        QSettings().setValue("assistant/speed", m_speed);
    applyPlaybackPreferences();
    emit preferencesChanged();
}

void AssistantVoice::setReplyMode(const QString &value) {
    if (!QStringList{"all", "voice", "text"}.contains(value))
        return;
    m_replyMode = value;
    if (!qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS"))
        QSettings().setValue("assistant/replyMode", value);
    if (value == "text" && !m_preview)
        stop();
    emit preferencesChanged();
}

void AssistantVoice::resetPlaybackPreferences() {
    setVolume(85);
    setSpeed(100);
    setReplyMode("all");
    m_failed = false;
    status(QStringLiteral("已恢复默认播报设置"));
}

void AssistantVoice::synthesize(const QString &text, const QString &reference, const QString &transcript, bool preview) {
    if (m_busy)
        return;
    m_failed = false;
    if (!configured()) {
        m_failed = true;
        status(QStringLiteral("语音功能暂不可用"));
        return;
    }
    if (!QFileInfo(reference).isFile() || transcript.isEmpty() || transcript.size() > 300) {
        m_failed = true;
        status(QStringLiteral("请准备参考音频，并填写 1 至 300 字的原文"));
        return;
    }
    cancelWakePromptPreparation();
    m_player.setSource({});
    if (!m_wave.isEmpty())
        QFile::remove(m_wave);
    m_wave = m_dir.filePath(QStringLiteral("reply-%1.wav").arg(++m_serial));
    m_pendingTranscript = transcript;
    m_preview = preview;
    m_busy = m_worker.submit({{"text", text}, {"reference", reference}, {"transcript", transcript},
                              {"output", m_wave}});
    m_failed = !m_busy;
    status(m_busy ? (preview ? QStringLiteral("正在生成试听…") : QStringLiteral("正在准备语音回复…"))
                  : QStringLiteral("暂时无法生成语音，请稍后再试"));
}

void AssistantVoice::complete(const QJsonObject &result) {
    if (m_wakePromptPreparing) {
        m_wakePromptPreparing = false;
        const auto seconds = result.value("seconds").toDouble();
        if (!result.contains("error") && seconds >= 0.1 && seconds <= 1.8 && QFileInfo(m_wakePromptFile).isFile()) {
            m_wakePrompt = QUrl::fromLocalFile(m_wakePromptFile);
            emit wakePromptChanged();
        }
        return;
    }
    m_busy = false;
    prepareWakePrompt();
    if (result.contains("error") || !QFileInfo(m_wave).isFile()) {
        m_failed = true;
        qWarning().noquote() << "Voice synthesis failed:" << result.value("error").toString("Missing output audio");
        status(QStringLiteral("语音生成失败，请重试"));
        return;
    }
    const auto device = m_route.device();
    if (device.isNull()) {
        m_failed = true;
        status(QStringLiteral("音频已生成，当前没有音频输出"));
        return;
    }
    if (m_preview)
        m_verifiedTranscript = m_pendingTranscript;
    m_output.setDevice(device);
    m_player.setSource(QUrl::fromLocalFile(m_wave));
    applyPlaybackPreferences();
    m_player.play();
    status(m_preview ? (m_dirty ? QStringLiteral("试听已生成，可以保存并启用") : QStringLiteral("试听已生成"))
                     : QString());
}

void AssistantVoice::status(const QString &message) {
    m_message = message;
    emit changed();
}
