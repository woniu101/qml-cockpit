#pragma once
#include <QByteArray>
#include <QStringList>
#include <vector>

namespace cockpit {
struct AudioSamples {
    std::vector<float> samples;
    int rate = 16000;
};
AudioSamples decodeAudio(const QByteArray &, const QStringList &, double maximum);
AudioSamples readWave(const QString &, double maximum = 15);
void writeWave(const QString &, const AudioSamples &);
std::vector<float> resample(const AudioSamples &, int rate = 16000);
} // namespace cockpit
