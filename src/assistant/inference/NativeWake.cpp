#include "InferenceEngine.h"
#include "NativeAudio.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>
#include <sherpa-onnx/c-api/c-api.h>
#include <stdexcept>

namespace cockpit {
class NativeWake final : public InferenceEngine {
    QString m_path, m_keyword;
    QJsonObject m_pronunciations;
    QSet<QString> m_tokens;
    const SherpaOnnxKeywordSpotter *m_spotter = nullptr;
    const SherpaOnnxOnlineStream *m_stream = nullptr;
    int m_streamId = -1;
    void clearStream() {
        if (m_stream)
            SherpaOnnxDestroyOnlineStream(m_stream);
        m_stream = nullptr;
    }
    void configure(const QString &word) {
        if (word == m_keyword && m_spotter)
            return;
        if (!QRegularExpression(QStringLiteral("^[\\x{3400}-\\x{9fff}]{4,8}$")).match(word).hasMatch())
            throw std::runtime_error("唤醒词需为 4 到 8 个汉字");
        QStringList phonemes;
        for (QChar ch : word) {
            const auto parts = m_pronunciations[QString(ch)].toArray();
            if (parts.isEmpty())
                throw std::runtime_error("发音表不支持该汉字");
            for (const auto &part : parts) {
                if (!m_tokens.contains(part.toString()))
                    throw std::runtime_error("模型不支持该唤醒词的读音");
                phonemes << part.toString();
            }
        }
        const auto keywords = (phonemes.join(' ') + " @" + word + "\n").toUtf8();
        auto file = [&](const char *name) { return QDir(m_path).filePath(name).toUtf8(); };
        const auto encoder = file("encoder-epoch-13-avg-2-chunk-8-left-64.int8.onnx"),
                   decoder = file("decoder-epoch-13-avg-2-chunk-8-left-64.onnx"),
                   joiner = file("joiner-epoch-13-avg-2-chunk-8-left-64.int8.onnx"),
                   tokens = file("tokens.txt");
        SherpaOnnxKeywordSpotterConfig config{};
        config.feat_config.sample_rate = 16000;
        config.feat_config.feature_dim = 80;
        config.model_config.transducer.encoder = encoder.constData();
        config.model_config.transducer.decoder = decoder.constData();
        config.model_config.transducer.joiner = joiner.constData();
        config.model_config.tokens = tokens.constData();
        config.model_config.provider = "cpu";
        config.model_config.num_threads = 2;
        config.max_active_paths = 4;
        config.num_trailing_blanks = 1;
        config.keywords_score = 1;
        config.keywords_threshold = 0.25f;
        config.keywords_buf = keywords.constData();
        config.keywords_buf_size = keywords.size();
        const auto *next = SherpaOnnxCreateKeywordSpotter(&config);
        if (!next)
            throw std::runtime_error("Cannot load keyword spotter");
        clearStream();
        if (m_spotter)
            SherpaOnnxDestroyKeywordSpotter(m_spotter);
        m_spotter = next;
        m_keyword = word;
    }
    bool decode(const SherpaOnnxOnlineStream *stream, const std::vector<float> &samples) {
        SherpaOnnxOnlineStreamAcceptWaveform(stream, 16000, samples.data(), int(samples.size()));
        while (SherpaOnnxIsKeywordStreamReady(m_spotter, stream)) {
            SherpaOnnxDecodeKeywordStream(m_spotter, stream);
            const auto *result = SherpaOnnxGetKeywordResult(m_spotter, stream);
            const bool hit = result && QString::fromUtf8(result->keyword) == m_keyword;
            if (result)
                SherpaOnnxDestroyKeywordResult(result);
            if (hit) {
                SherpaOnnxResetKeywordStream(m_spotter, stream);
                return true;
            }
        }
        return false;
    }

  public:
    explicit NativeWake(const QString &path) : m_path(path) {
        QFile dict(QDir(path).filePath("pronunciations.json"));
        if (!dict.open(QIODevice::ReadOnly))
            throw std::runtime_error("Missing exported wake pronunciations.json");
        m_pronunciations = QJsonDocument::fromJson(dict.readAll()).object();
        QFile tokens(QDir(path).filePath("tokens.txt"));
        if (!tokens.open(QIODevice::ReadOnly))
            throw std::runtime_error("Missing keyword tokens");
        for (const auto &line : QString::fromUtf8(tokens.readAll()).split('\n'))
            m_tokens.insert(line.section(' ', 0, 0));
    }
    ~NativeWake() override {
        clearStream();
        if (m_spotter)
            SherpaOnnxDestroyKeywordSpotter(m_spotter);
    }
    QJsonObject run(const QJsonObject &r, const std::atomic_bool &cancelled) override {
        configure(r["keyword"].toString());
        const auto operation = r["operation"].toString();
        if (operation == "configure")
            return {{"ok", true}};
        const auto raw = QByteArray::fromBase64(r["audio"].toString().toLatin1());
        const auto audio = operation == "replay"
                               ? decodeAudio(raw, {"wav"}, 10)
                               : decodeAudio(raw,
                                             {r["encoding"].toString(), QString::number(r["rate"].toInt()),
                                              QString::number(r["channels"].toInt())},
                                             10);
        auto samples = resample(audio);
        if (cancelled)
            return {{"ok", false}};
        if (operation == "replay") {
            const auto *stream = SherpaOnnxCreateKeywordStream(m_spotter);
            if (!stream)
                throw std::runtime_error("Cannot create keyword stream");
            samples.insert(samples.end(), 16000, 0);
            const bool hit = decode(stream, samples);
            SherpaOnnxDestroyOnlineStream(stream);
            return {{"ok", true}, {"hit", hit}};
        }
        if (operation != "feed")
            throw std::runtime_error("Unknown keyword operation");
        if (!m_stream || m_streamId != r["stream"].toInt()) {
            clearStream();
            m_stream = SherpaOnnxCreateKeywordStream(m_spotter);
            m_streamId = r["stream"].toInt();
        }
        if (!m_stream)
            throw std::runtime_error("Cannot create keyword stream");
        return {{"ok", true}, {"hit", decode(m_stream, samples)}};
    }
};
std::unique_ptr<InferenceEngine> makeWake(const QString &path) { return std::make_unique<NativeWake>(path); }
} // namespace cockpit
