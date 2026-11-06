#pragma once

#include <QAudioFormat>
#include <QByteArrayView>
#include <cmath>

class AssistantEndpoint {
public:
    enum class Result { Listening, Silence, Finished };

    Result observe(QByteArrayView samples, const QAudioFormat &format) {
        const int frames = int(samples.size()) / format.bytesPerFrame();
        double sum = 0;
        for (qsizetype i = 0; i < samples.size(); i += format.bytesPerSample()) {
            const float sample = format.normalizedSampleValue(samples.data() + i);
            sum += sample * sample;
        }
        m_total += frames;
        if (sum > 0.000225 * frames * format.channelCount()) {
            m_quiet = 0;
            m_loud += frames;
            if (m_loud >= format.sampleRate() / 10)
                m_spoke = true;
        } else {
            m_loud = 0;
            m_quiet += frames;
        }
        if (m_spoke && m_quiet >= format.sampleRate() * 7 / 10)
            return Result::Finished;
        if (!m_spoke && m_total >= format.sampleRate() * 4)
            return Result::Silence;
        return Result::Listening;
    }

    bool spoke() const { return m_spoke; }

private:
    int m_total = 0;
    int m_loud = 0;
    int m_quiet = 0;
    bool m_spoke = false;
};
