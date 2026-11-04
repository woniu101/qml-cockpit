#include "AssistantVoice.h"

#include <QAudioDevice>
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

AssistantVoice::AssistantVoice(QAudioOutput &mediaOutput)
    : m_mediaOutput(mediaOutput),
      m_worker(qEnvironmentVariable("CAR_TERMINAL_ASR_PYTHON"), qEnvironmentVariable("CAR_TERMINAL_TTS_WORKER"),
               qEnvironmentVariable("CAR_TERMINAL_TTS_MODEL"), 60000) {
    m_player.setAudioOutput(&m_output);
    if (!qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS")) {
        QSettings settings;
        m_volume = std::clamp(settings.value("assistant/volume", 85).toInt(), 0, 100);
        m_speed = settings.value("assistant/speed", 100).toInt();
        if (m_speed != 85 && m_speed != 115)
            m_speed = 100;
        m_repliesEnabled = settings.value("assistant/replies", true).toBool();
    }
    if (!rateAvailable())
        m_speed = 100;
    applyPlaybackPreferences();
    connect(&m_player, &QMediaPlayer::playbackStateChanged, this, &AssistantVoice::changed);
    connect(&m_player, &QMediaPlayer::errorOccurred, this, [this] {
        status(QStringLiteral("ZipVoice 音频已生成，当前输出无法播放"));
    });
    connect(&m_worker, &AssistantWorker::completed, this, &AssistantVoice::complete);
    if (!qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS"))
        m_storage = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/assistant/voice.json";
    const auto saved = QJsonDocument::fromJson(readBounded(m_storage, 2 * MaxWav)).object();
    const auto savedText = saved.value("transcript").toString().trimmed();
    if (m_dir.isValid() && !savedText.isEmpty() &&
        writeFile(m_dir.filePath("voice.wav"), QByteArray::fromBase64(saved.value("wave").toString().toLatin1()))) {
        m_reference = m_dir.filePath("voice.wav");
        m_referenceText = savedText;
        m_draftName = saved.value("name").toString();
    } else {
        m_reference = qEnvironmentVariable("CAR_TERMINAL_TTS_REFERENCE");
        m_referenceText = QString::fromUtf8(readBounded(qEnvironmentVariable("CAR_TERMINAL_TTS_TRANSCRIPT"), 2048)).trimmed();
        if (m_referenceText.startsWith(QChar(0xfeff)))
            m_referenceText.remove(0, 1);
        if (!m_reference.isEmpty())
            m_draftName = QStringLiteral("开发默认音色");
    }
    m_draft = m_reference;
    m_activeName = m_draftName;
    m_draftTranscript = m_referenceText;
    m_message = configured() ? QStringLiteral("ZipVoice · 试听后可保存音色") : QStringLiteral("ZipVoice 模型尚未配置");
}

void AssistantVoice::importReference(const QUrl &url) {
    if (m_busy || !configured())
        return;
    const QFileInfo info(url.toLocalFile());
    if (!url.isLocalFile() || info.suffix().compare("wav", Qt::CaseInsensitive) != 0 ||
        !writeFile(m_dir.filePath("draft.wav"), readBounded(info.absoluteFilePath(), MaxWav))) {
        status(QStringLiteral("请选择有效的本地 WAV 参考音频"));
        return;
    }
    m_draft = m_dir.filePath("draft.wav");
    m_draftName = info.fileName();
    m_draftTranscript.clear();
    m_verifiedTranscript.clear();
    status(QStringLiteral("请填写音频中的原话，再试听确认"));
}

void AssistantVoice::preview(const QString &transcript) {
    m_verifiedTranscript.clear();
    m_draftTranscript = transcript.trimmed();
    synthesize(QStringLiteral("右后车窗已调到百分之三十。"), m_draft, m_draftTranscript, true);
}

bool AssistantVoice::save(const QString &transcript) {
    if (m_busy || !verified() || transcript.trimmed() != m_verifiedTranscript)
        return false;
    const auto wav = readBounded(m_draft, MaxWav);
    const QJsonObject profile{{"name", m_draftName}, {"transcript", m_verifiedTranscript},
                              {"wave", QString::fromLatin1(wav.toBase64())}};
    if (m_storage.isEmpty() || wav.isEmpty() || !QDir().mkpath(QFileInfo(m_storage).absolutePath()) ||
        !writeFile(m_storage, QJsonDocument(profile).toJson(QJsonDocument::Compact))) {
        status(QStringLiteral("音色保存失败，请检查本地存储"));
        return false;
    }
    const auto activeReference = m_dir.filePath("voice.wav");
    if (!writeFile(activeReference, wav)) {
        status(QStringLiteral("音色已保存，重启助手后生效"));
        return true;
    }
    m_reference = activeReference;
    m_referenceText = m_verifiedTranscript;
    m_activeName = m_draftName;
    status(QStringLiteral("音色已保存，并用于助手回复"));
    return true;
}

void AssistantVoice::stop() {
    m_queuedReply.clear();
    if (synthesizing())
        m_discard = true;
    m_player.stop();
    if (m_busy)
        status(QStringLiteral("已停止本次播报"));
}

void AssistantVoice::speak(const QString &text) {
    if (!m_repliesEnabled || !configured() || m_reference.isEmpty())
        return;
    if (m_busy) {
        if (!m_preview) {
            m_queuedReply = text;
            m_discard = true;
            emit changed();
        }
        return;
    }
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
    if (!m_storage.isEmpty())
        QSettings().setValue("assistant/volume", m_volume);
    applyPlaybackPreferences();
    emit preferencesChanged();
}

void AssistantVoice::setSpeed(int value) {
    if (!rateAvailable() || (value != 85 && value != 100 && value != 115))
        return;
    m_speed = value;
    if (!m_storage.isEmpty())
        QSettings().setValue("assistant/speed", m_speed);
    applyPlaybackPreferences();
    emit preferencesChanged();
}

void AssistantVoice::setRepliesEnabled(bool value) {
    m_repliesEnabled = value;
    if (!m_storage.isEmpty())
        QSettings().setValue("assistant/replies", value);
    if (!value && !m_preview)
        stop();
    emit preferencesChanged();
}

void AssistantVoice::resetPlaybackPreferences() {
    setVolume(85);
    setSpeed(100);
    setRepliesEnabled(true);
}

void AssistantVoice::synthesize(const QString &text, const QString &reference, const QString &transcript, bool preview) {
    if (m_busy)
        return;
    if (!configured() || !QFileInfo(reference).isFile() || transcript.isEmpty() || transcript.size() > 300) {
        status(QStringLiteral("请选择参考 WAV，并填写 1 至 300 字的原文"));
        return;
    }
    m_player.setSource({});
    QFile::remove(m_wave);
    m_wave = m_dir.filePath(QStringLiteral("reply-%1.wav").arg(++m_serial));
    m_pendingTranscript = transcript;
    m_preview = preview;
    m_discard = false;
    m_busy = m_worker.submit({{"text", text.left(200)}, {"reference", reference}, {"transcript", transcript},
                              {"output", m_wave}});
    status(m_busy ? QStringLiteral("ZipVoice 正在合成…") : QStringLiteral("无法启动 ZipVoice"));
}

void AssistantVoice::complete(const QJsonObject &result) {
    m_busy = false;
    if (m_discard) {
        const auto next = std::exchange(m_queuedReply, {});
        if (next.isEmpty())
            status(QStringLiteral("已取消本次播报"));
        else
            speak(next);
        return;
    }
    if (result.contains("error") || !QFileInfo(m_wave).isFile()) {
        status(QStringLiteral("ZipVoice 合成失败：%1").arg(result.value("error").toString()));
        return;
    }
    if (m_preview)
        m_verifiedTranscript = m_pendingTranscript;
    const auto device = m_mediaOutput.device();
    if (device.isNull()) {
        status(QStringLiteral("音频已生成，当前没有音频输出"));
        return;
    }
    m_output.setDevice(device);
    m_player.setSource(QUrl::fromLocalFile(m_wave));
    applyPlaybackPreferences();
    m_player.play();
    status(QStringLiteral("ZipVoice 已生成 · %1 ms").arg(result.value("elapsed_ms").toInt()));
}

void AssistantVoice::status(const QString &message) {
    m_message = message;
    emit changed();
}
