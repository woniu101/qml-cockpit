#include "InferenceEngine.h"
#include "NativeAudio.h"
#include <QDir>
#include <QJsonArray>
#include <sherpa-onnx/c-api/c-api.h>
#include <stdexcept>

namespace cockpit {
class NativeAsr final : public InferenceEngine {
    const SherpaOnnxOfflineRecognizer *m_recognizer = nullptr;

  public:
    explicit NativeAsr(const QString &path) {
        const auto model = QDir(path).filePath("model.int8.onnx").toUtf8(),
                   tokens = QDir(path).filePath("tokens.txt").toUtf8();
        SherpaOnnxOfflineRecognizerConfig config{};
        config.feat_config.sample_rate = 16000;
        config.feat_config.feature_dim = 80;
        config.model_config.paraformer.model = model.constData();
        config.model_config.tokens = tokens.constData();
        config.model_config.num_threads = 2;
        config.model_config.provider = "cpu";
        config.decoding_method = "greedy_search";
        m_recognizer = SherpaOnnxCreateOfflineRecognizer(&config);
        if (!m_recognizer)
            throw std::runtime_error("Cannot load Paraformer model");
    }
    ~NativeAsr() override { SherpaOnnxDestroyOfflineRecognizer(m_recognizer); }
    QJsonObject run(const QJsonObject &request, const std::atomic_bool &cancelled) override {
        QStringList format;
        for (const auto &v : request["format"].toArray())
            format << v.toString();
        const auto audio = decodeAudio(QByteArray::fromBase64(request["audio"].toString().toLatin1()), format,
                                       request["reference"].toBool() ? 15 : 10);
        const auto samples = resample(audio);
        const auto *stream = SherpaOnnxCreateOfflineStream(m_recognizer);
        if (!stream)
            throw std::runtime_error("Cannot create ASR stream");
        SherpaOnnxAcceptWaveformOffline(stream, 16000, samples.data(), int(samples.size()));
        if (!cancelled)
            SherpaOnnxDecodeOfflineStream(m_recognizer, stream);
        const auto *result = SherpaOnnxGetOfflineStreamResult(stream);
        const auto text = result ? QString::fromUtf8(result->text).trimmed() : QString();
        if (result)
            SherpaOnnxDestroyOfflineRecognizerResult(result);
        SherpaOnnxDestroyOfflineStream(stream);
        return {{"text", text}};
    }
};
std::unique_ptr<InferenceEngine> makeAsr(const QString &path) { return std::make_unique<NativeAsr>(path); }
} // namespace cockpit
