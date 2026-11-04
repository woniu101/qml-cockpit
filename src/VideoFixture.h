#pragma once
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDataStream>

// Original deterministic, silent 160x90 AVI for real decoding/seek tests.
inline bool writeVideoFixture(const QString &path) {
    constexpr int width = 160, height = 90, fps = 10, frames = 180, frameSize = width * height * 3;
    auto integers = [](std::initializer_list<quint32> values) {
        QByteArray bytes;
        QDataStream stream(&bytes, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        for (auto value : values)
            stream << value;
        return bytes;
    };
    auto chunk = [&](const char *id, const QByteArray &data) {
        QByteArray value(id, 4);
        value += integers({quint32(data.size())});
        value += data;
        if (data.size() % 2)
            value += '\0';
        return value;
    };
    auto list = [&](const char *kind, const QByteArray &data) {
        return chunk("LIST", QByteArray(kind, 4) + data);
    };
    const auto avih = integers(
        {1000000 / fps, frameSize * fps, 0, 0x10, frames, 0, 1, frameSize, width, height, 0, 0, 0, 0});
    QByteArray strh("vidsDIB ", 8);
    strh += integers({0, 0, 0, 1, fps, 0, frames, frameSize, 0xffffffff, 0});
    QDataStream region(&strh, QIODevice::Append);
    region.setByteOrder(QDataStream::LittleEndian);
    region << qint16(0) << qint16(0) << qint16(width) << qint16(height);
    const auto strf = integers({40, width, height, quint32(24 << 16 | 1), 0, frameSize, 0, 0, 0, 0});
    const auto header =
        list("hdrl", chunk("avih", avih) + list("strl", chunk("strh", strh) + chunk("strf", strf)));
    QByteArray movie, index;
    movie.reserve(frames * (frameSize + 8));
    for (int frame = 0; frame < frames; ++frame) {
        QByteArray pixels(frameSize, '\0');
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x) {
                const int at = (y * width + x) * 3;
                const bool moving = x > frame % width && x < frame % width + 12;
                pixels[at] = char(moving ? 190 : 105 + y / 2);
                pixels[at + 1] = char(moving ? 215 : 135 + x / 4);
                pixels[at + 2] = char(moving ? 235 : 75 + y / 3);
            }
        index += QByteArray("00db", 4) + integers({0x10, quint32(movie.size() + 4), frameSize});
        movie += chunk("00db", pixels);
    }
    const auto body = QByteArray("AVI ", 4) + header + list("movi", movie) + chunk("idx1", index);
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(chunk("RIFF", body)) == body.size() + 8;
}
