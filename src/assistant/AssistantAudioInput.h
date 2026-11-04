#pragma once

#include <QAudioDevice>
#include <QString>

inline QAudioFormat assistantInputFormat(const QAudioDevice &device) {
    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    return device.isFormatSupported(format) ? format : device.preferredFormat();
}

inline QString assistantInputEncoding(const QAudioFormat &format) {
    switch (format.sampleFormat()) {
    case QAudioFormat::Int16: return "s16";
    case QAudioFormat::Int32: return "s32";
    case QAudioFormat::Float: return "f32";
    case QAudioFormat::UInt8: return "u8";
    default: return {};
    }
}
