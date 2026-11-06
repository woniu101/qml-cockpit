#include "NativeAudio.h"
#include <QDataStream>
#include <QFile>
#include <QSaveFile>
#include <QtEndian>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sherpa-onnx/c-api/c-api.h>
#include <stdexcept>

namespace cockpit {
AudioSamples decodeAudio(const QByteArray &input, const QStringList &format, double maximum) {
    QByteArray raw = input;
    QString encoding;
    int rate = 0, channels = 0;
    if (format == QStringList{"wav"}) {
        if (raw.size() < 44 || raw.first(4) != "RIFF" || raw.mid(8, 4) != "WAVE")
            throw std::runtime_error("Invalid WAV");
        QByteArray data;
        bool fmt = false, found = false;
        for (qint64 pos = 12; pos + 8 <= raw.size();) {
            const quint32 size = qFromLittleEndian<quint32>(raw.constData() + pos + 4);
            if (size > quint32(raw.size() - pos - 8))
                throw std::runtime_error("Truncated WAV");
            const auto id = raw.mid(pos, 4);
            const auto *p = raw.constData() + pos + 8;
            if (id == "fmt " && size >= 16) {
                const int codec = qFromLittleEndian<quint16>(p);
                channels = qFromLittleEndian<quint16>(p + 2);
                rate = qFromLittleEndian<quint32>(p + 4);
                const int bits = qFromLittleEndian<quint16>(p + 14);
                encoding = codec == 3 && bits == 32   ? "f32"
                           : codec == 1 && bits == 16 ? "s16"
                           : codec == 1 && bits == 32 ? "s32"
                           : codec == 1 && bits == 8  ? "u8"
                                                      : "";
                fmt = true;
            } else if (id == "data") {
                data = QByteArray(p, size);
                found = true;
            }
            pos += 8 + size + (size & 1);
        }
        if (!fmt || !found)
            throw std::runtime_error("Missing WAV samples");
        raw = data;
    } else if (format.size() == 3) {
        encoding = format[0];
        rate = format[1].toInt();
        channels = format[2].toInt();
    }
    const int width = encoding == "s16"                        ? 2
                      : encoding == "s32" || encoding == "f32" ? 4
                      : encoding == "u8"                       ? 1
                                                               : 0;
    if (!width || rate < 8000 || rate > 192000 || channels < 1 || channels > 8 ||
        raw.size() % (channels * width))
        throw std::runtime_error("Unsupported audio format");
    const auto frames = raw.size() / (channels * width);
    if (frames < rate / 5 || frames > rate * maximum)
        throw std::runtime_error("Unexpected audio length");
    AudioSamples result;
    result.rate = rate;
    result.samples.resize(frames);
    for (qsizetype i = 0; i < frames; ++i) {
        float sum = 0;
        for (int c = 0; c < channels; ++c) {
            const char *p = raw.constData() + (i * channels + c) * width;
            float value = 0;
            if (encoding == "s16")
                value = qFromLittleEndian<qint16>(p) / 32768.0f;
            else if (encoding == "s32")
                value = qFromLittleEndian<qint32>(p) / 2147483648.0f;
            else if (encoding == "u8")
                value = (quint8(*p) - 128) / 128.0f;
            else {
                const auto bits = qFromLittleEndian<quint32>(p);
                std::memcpy(&value, &bits, 4);
            }
            if (!std::isfinite(value))
                throw std::runtime_error("Invalid audio sample");
            sum += value;
        }
        result.samples[i] = std::clamp(sum / channels, -1.0f, 1.0f);
    }
    return result;
}
AudioSamples readWave(const QString &path, double maximum) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 32 * 1024 * 1024)
        throw std::runtime_error("Cannot read audio");
    return decodeAudio(file.readAll(), {"wav"}, maximum);
}
std::vector<float> resample(const AudioSamples &audio, int rate) {
    if (audio.rate == rate)
        return audio.samples;
    const auto *filter =
        SherpaOnnxCreateLinearResampler(audio.rate, rate, 0.99f * 0.5f * std::min(rate, audio.rate), 6);
    if (!filter)
        throw std::runtime_error("Cannot resample audio");
    const auto *out =
        SherpaOnnxLinearResamplerResample(filter, audio.samples.data(), int(audio.samples.size()), 1);
    if (!out) {
        SherpaOnnxDestroyLinearResampler(filter);
        throw std::runtime_error("Resampling failed");
    }
    std::vector<float> result(out->samples, out->samples + out->n);
    SherpaOnnxLinearResamplerResampleFree(out);
    SherpaOnnxDestroyLinearResampler(filter);
    return result;
}
void writeWave(const QString &path, const AudioSamples &audio) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        throw std::runtime_error("Cannot write audio");
    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.writeRawData("RIFF", 4);
    stream << quint32(36 + audio.samples.size() * 2);
    stream.writeRawData("WAVEfmt ", 8);
    stream << quint32(16) << quint16(1) << quint16(1) << quint32(audio.rate) << quint32(audio.rate * 2)
           << quint16(2) << quint16(16);
    stream.writeRawData("data", 4);
    stream << quint32(audio.samples.size() * 2);
    for (float x : audio.samples) {
        if (!std::isfinite(x))
            throw std::runtime_error("Invalid synthesized sample");
        stream << qint16(std::clamp(x, -1.0f, 1.0f) * 32767);
    }
    if (stream.status() != QDataStream::Ok || !file.commit())
        throw std::runtime_error("Cannot save audio");
}
} // namespace cockpit
