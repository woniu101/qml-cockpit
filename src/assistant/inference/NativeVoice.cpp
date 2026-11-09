#include "InferenceEngine.h"
#include "NativeAudio.h"
#include "VoiceText.h"
#include <QDir>
#include <QRegularExpression>
#include <sherpa-onnx/c-api/c-api.h>
#include <algorithm>
#include <stdexcept>

namespace cockpit {
class NativeVoice final : public InferenceEngine {
    const SherpaOnnxOfflineTts *m_tts = nullptr;

  public:
    explicit NativeVoice(const QString &path) {
        auto file = [&](const char *name) { return QDir(path).filePath(name).toUtf8(); };
        const auto encoder = file("encoder.int8.onnx"), decoder = file("decoder.int8.onnx"),
                   tokens = file("tokens.txt"), lexicon = file("lexicon.txt"),
                   vocoder = file("vocos_24khz.onnx");
        SherpaOnnxOfflineTtsConfig config{};
        config.model.num_threads = 4;
        config.model.provider = "cpu";
        auto &model = config.model.zipvoice;
        model.encoder = encoder.constData();
        model.decoder = decoder.constData();
        model.tokens = tokens.constData();
        model.lexicon = lexicon.constData();
        model.vocoder = vocoder.constData();
        config.max_num_sentences = 1;
        config.silence_scale = 0.2f;
        m_tts = SherpaOnnxCreateOfflineTts(&config);
        if (!m_tts)
            throw std::runtime_error("Cannot load ZipVoice model");
    }
    ~NativeVoice() override { SherpaOnnxDestroyOfflineTts(m_tts); }
    QJsonObject run(const QJsonObject &request, const std::atomic_bool &cancelled) override {
        const auto reference = readWave(request["reference"].toString());
        if (reference.samples.size() < size_t(reference.rate))
            throw std::runtime_error("Reference must be 1-15 seconds");
        float peak = 0;
        for (float v : reference.samples)
            peak = std::max(peak, std::abs(v));
        if (peak < 0.005f)
            throw std::runtime_error("Reference too quiet");
        const auto text = request["text"].toString();
        if (text.trimmed().isEmpty() || text.size() > 1024)
            throw std::runtime_error("Reply length must be 1-1024 characters");
        const auto transcript = request["transcript"].toString().toUtf8();
        SherpaOnnxGenerationConfig config{};
        config.speed = 1;
        config.silence_scale = 0.2f;
        config.num_steps = 4;
        config.reference_audio = reference.samples.data();
        config.reference_audio_len = int(reference.samples.size());
        config.reference_sample_rate = reference.rate;
        config.reference_text = transcript.constData();
        AudioSamples output;
        output.rate = SherpaOnnxOfflineTtsSampleRate(m_tts);
        const QRegularExpression clause(
            QStringLiteral("[^。！？!?；;，,：:\\n]+[。！？!?；;，,：:\\n]*|[。！？!?；;，,：:\\n]+"));
        auto matches = clause.globalMatch(voiceTextForSynthesis(text));
        int count = 0;
        while (matches.hasNext()) {
            auto phrase = matches.next().captured();
            while (!phrase.isEmpty()) {
                const auto chunk = phrase.left(48).toUtf8();
                phrase.remove(0, 48);
                if (cancelled)
                    return {{"error", "cancelled"}};
                const auto *audio = SherpaOnnxOfflineTtsGenerateWithConfig(
                    m_tts, chunk.constData(), &config,
                    [](const float *, int32_t, float, void *p) -> int32_t {
                        return !*static_cast<const std::atomic_bool *>(p);
                    },
                    const_cast<std::atomic_bool *>(&cancelled));
                if (!audio)
                    throw std::runtime_error("ZipVoice generation failed");
                if (audio->sample_rate != output.rate || audio->n <= 0) {
                    SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
                    throw std::runtime_error("Invalid ZipVoice output");
                }
                if (count++)
                    output.samples.insert(output.samples.end(), output.rate / 10, 0);
                output.samples.insert(output.samples.end(), audio->samples, audio->samples + audio->n);
                SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
            }
        }
        if (cancelled)
            return {{"error", "cancelled"}};
        const auto fade = std::min(output.samples.size(), size_t(output.rate * 16 / 1000));
        auto tail = output.samples.end() - fade;
        for (size_t i = 0; i < fade; ++i)
            tail[i] *= float(fade - i - 1) / float(fade);
        if (!request.value("wakePrompt").toBool())
            output.samples.resize(output.samples.size() + output.rate * 180 / 1000, 0.0f);
        writeWave(request["output"].toString(), output);
        return {{"seconds", double(output.samples.size()) / output.rate}, {"segments", count}};
    }
};
std::unique_ptr<InferenceEngine> makeVoice(const QString &path) {
    return std::make_unique<NativeVoice>(path);
}
} // namespace cockpit
