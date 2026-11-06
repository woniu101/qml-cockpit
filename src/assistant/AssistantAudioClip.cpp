#include "AssistantAudioClip.h"
#include "AssistantAudioInput.h"

#include <QAudioBuffer>
#include <QAudioSource>
#include <QDataStream>
#include <QFileInfo>
#include <QSaveFile>
#include <QtEndian>
#include <algorithm>
#include <cmath>

namespace {
bool writeWave(const QString &path, const QByteArray &samples, const QAudioFormat &format) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.writeRawData("RIFF", 4);
    stream << quint32(36 + samples.size());
    stream.writeRawData("WAVEfmt ", 8);
    stream << quint32(16) << quint16(format.sampleFormat() == QAudioFormat::Float ? 3 : 1)
           << quint16(format.channelCount()) << quint32(format.sampleRate())
           << quint32(format.sampleRate() * format.bytesPerFrame())
           << quint16(format.bytesPerFrame()) << quint16(format.bytesPerSample() * 8);
    stream.writeRawData("data", 4);
    stream << quint32(samples.size());
    stream.writeRawData(samples.constData(), samples.size());
    return stream.status() == QDataStream::Ok && file.commit();
}
}

AssistantAudioClip::AssistantAudioClip(double sourceLimit, double minimum, double clipLimit)
    : m_sourceLimit(sourceLimit), m_minimum(minimum), m_clipLimit(clipLimit) {
    m_timeout.setSingleShot(true);
    connect(&m_timeout, &QTimer::timeout, this, [this] {
        if (recording())
            finishRecording();
        else
            fail(QStringLiteral("音频读取超时，请重试"));
    });
    m_meter.setInterval(80);
    connect(&m_meter, &QTimer::timeout, this, [this] {
        const auto bytes = m_recorded.data().mid(m_offset);
        m_offset += bytes.size();
        double peak = 0;
        for (qsizetype i = 0; i + m_format.bytesPerSample() <= bytes.size(); i += m_format.bytesPerSample())
            peak = std::max(peak, double(std::abs(m_format.normalizedSampleValue(bytes.constData() + i))));
        m_level = std::min(1.0, peak * 2);
        emit changed();
    });
}

AssistantAudioClip::~AssistantAudioClip() { stop(); }

QUrl AssistantAudioClip::wave() const { return QUrl::fromLocalFile(m_wave); }

void AssistantAudioClip::stop() {
    m_timeout.stop();
    m_meter.stop();
    if (m_decoder) {
        auto *decoder = m_decoder.release();
        disconnect(decoder, nullptr, this, nullptr);
        decoder->stop();
        decoder->deleteLater();
    }
    if (m_capture) {
        disconnect(m_capture.get(), nullptr, this, nullptr);
        m_capture->stop();
        m_capture.reset();
    }
    m_recorded.close();
    m_busy = false;
    m_level = 0;
}

void AssistantAudioClip::cancel() {
    ++m_revision;
    stop();
    m_wave.clear();
    m_pcm.clear();
    m_start = m_length = 0;
    m_message.clear();
    m_decodeError.clear();
    emit changed();
}

void AssistantAudioClip::fail(const QString &message) {
    stop();
    m_wave.clear();
    m_message = message;
    emit changed();
    emit failed(message);
}

void AssistantAudioClip::load(const QUrl &url) {
    cancel();
    const QFileInfo file(url.toLocalFile());
    if (!url.isLocalFile() || !file.isFile() || file.size() <= 0 || file.size() > 32 * 1024 * 1024 ||
        !QStringList{"wav", "mp3"}.contains(file.suffix().toLower())) {
        fail(QStringLiteral("请选择不超过 32 MB 的本地 WAV 或 MP3 音频"));
        return;
    }
    m_name = file.fileName();
    m_decoder = std::make_unique<QAudioDecoder>();
    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    m_decoder->setAudioFormat(format);
    const auto decoder = m_decoder.get();
    const int revision = m_revision;
    connect(m_decoder.get(), &QAudioDecoder::bufferReady, this, [this, decoder, revision] {
        if (m_revision != revision || m_decoder.get() != decoder)
            return;
        const auto audio = m_decoder->read();
        append(QByteArray(audio.constData<char>(), audio.byteCount()), audio.format());
    });
    connect(m_decoder.get(), &QAudioDecoder::finished, this, [this, decoder, revision] {
        if (m_revision == revision && m_decoder.get() == decoder)
            finish();
    });
    connect(m_decoder.get(), qOverload<QAudioDecoder::Error>(&QAudioDecoder::error), this, [this, decoder, revision] {
        if (m_revision != revision || m_decoder.get() != decoder)
            return;
        const auto message = m_decoder->errorString();
        QTimer::singleShot(0, this, [this, message, revision] {
            if (m_busy && revision == m_revision) fail(QStringLiteral("无法解码音频 · ") + message);
        });
    });
    m_decoder->setSource(url);
    m_busy = true;
    m_message = QStringLiteral("正在读取音频…");
    m_timeout.start(30000);
    m_decoder->start();
    emit changed();
}

void AssistantAudioClip::append(const QByteArray &bytes, const QAudioFormat &format) {
    if (!m_decodeError.isEmpty())
        return;
    if (!format.isValid() || format.sampleRate() != 16000 || format.channelCount() != 1 ||
        format.sampleFormat() != QAudioFormat::Int16) {
        m_decodeError = QStringLiteral("音频转换失败，请选择其他音频");
        return;
    }
    if (m_pcm.size() + bytes.size() > qint64(m_sourceLimit * 32000)) {
        m_decodeError = QStringLiteral("音频超过 %1 秒，请选择较短片段").arg(m_sourceLimit);
        const int revision = m_revision;
        QTimer::singleShot(0, this, [this, revision] {
            if (m_busy && revision == m_revision) fail(m_decodeError);
        });
        return;
    }
    m_pcm.append(bytes);
}

void AssistantAudioClip::finish() {
    stop();
    if (!m_decodeError.isEmpty()) {
        fail(m_decodeError);
        return;
    }
    if (duration() < m_minimum) {
        fail(QStringLiteral("有效音频至少需要 %1 秒").arg(m_minimum));
        return;
    }
    if (select(0, std::min(duration(), m_clipLimit)))
        emit loaded();
    else
        emit failed(m_message);
}

bool AssistantAudioClip::select(double start, double length) {
    if (m_busy || !std::isfinite(start) || !std::isfinite(length) || start < 0 || length < m_minimum ||
        length > m_clipLimit || start + length > duration() + 0.001)
        return false;
    const auto selected = m_pcm.mid(qRound64(start * 16000) * 2, qRound64(length * 16000) * 2);
    int peak = 0;
    for (qsizetype i = 0; i + 1 < selected.size(); i += 2)
        peak = std::max(peak, std::abs(int(qFromLittleEndian<qint16>(selected.constData() + i))));
    if (peak < 164) {
        m_wave.clear();
        m_message = QStringLiteral("片段声音过轻或没有声音，请重新选取或录制");
        emit changed();
        return false;
    }
    const auto path = m_directory.filePath("clip.wav");
    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    if (!m_directory.isValid() || !writeWave(path, selected, format)) {
        fail(QStringLiteral("无法保存音频片段"));
        return false;
    }
    m_start = start;
    m_length = length;
    m_wave = path;
    m_message = QStringLiteral("已选 %1 秒，共 %2 秒").arg(length, 0, 'f', 1).arg(duration(), 0, 'f', 1);
    emit changed();
    return true;
}

void AssistantAudioClip::startRecording(const QAudioDevice &device) {
    cancel();
    if (device.isNull()) {
        fail(QStringLiteral("收音设备不可用，请检查设备设置"));
        return;
    }
    m_format = assistantInputFormat(device);
    if (!m_format.isValid() || assistantInputEncoding(m_format).isEmpty()
        || m_format.sampleRate() > 96000 || m_format.channelCount() > 2) {
        fail(QStringLiteral("当前麦克风不支持此录音格式，请使用导入音频"));
        return;
    }
    m_recorded.setData({});
    m_recorded.open(QIODevice::WriteOnly);
    m_offset = 0;
    m_capture = std::make_unique<QAudioSource>(device, m_format);
    connect(m_capture.get(), &QAudioSource::stateChanged, this, [this](QtAudio::State state) {
        if (m_capture && state == QtAudio::StoppedState && m_capture->error() != QtAudio::NoError) {
            const int revision = m_revision;
            QTimer::singleShot(0, this, [this, revision] {
                if (m_capture && revision == m_revision) fail(QStringLiteral("无法录音，请检查麦克风权限和连接"));
            });
        }
    });
    m_name = QStringLiteral("朗读录音");
    m_message = QStringLiteral("正在录制，请朗读提示文本");
    m_busy = true;
    m_timeout.start(15000);
    m_meter.start();
    m_capture->start(&m_recorded);
    emit changed();
}

void AssistantAudioClip::finishRecording() {
    if (!m_capture)
        return;
    stop();
    const auto path = m_directory.filePath(QStringLiteral("朗读录音.wav"));
    if (!writeWave(path, m_recorded.data(), m_format)) {
        fail(QStringLiteral("无法保存录音，请重试"));
        return;
    }
    load(QUrl::fromLocalFile(path));
}
