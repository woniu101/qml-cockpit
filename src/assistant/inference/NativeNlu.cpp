#include "BertTokenizer.h"
#include "BioDecoder.h"
#include "InferenceEngine.h"
#include "NluSlots.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QScopeGuard>
#include <algorithm>
#include <cmath>
#include <onnxruntime_c_api.h>
#include <stdexcept>

namespace cockpit {
namespace {
QJsonObject json(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Cannot read NLU configuration");
    const auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        throw std::runtime_error("Invalid NLU configuration");
    return doc.object();
}
} // namespace
class NativeNlu final : public InferenceEngine {
    const OrtApi *m_api = OrtGetApiBase()->GetApi(ORT_API_VERSION);
    OrtEnv *m_env = nullptr;
    OrtSessionOptions *m_options = nullptr;
    OrtSession *m_session = nullptr;
    OrtMemoryInfo *m_memory = nullptr;
    BertTokenizer m_tokenizer;
    QJsonObject m_labels;
    QString m_name;
    double m_threshold = 0;
    void check(OrtStatus *status) const {
        if (!status)
            return;
        const auto message = QString::fromUtf8(m_api->GetErrorMessage(status));
        m_api->ReleaseStatus(status);
        throw std::runtime_error(message.toStdString());
    }
    void release() {
        if (m_memory)
            m_api->ReleaseMemoryInfo(m_memory);
        if (m_session)
            m_api->ReleaseSession(m_session);
        if (m_options)
            m_api->ReleaseSessionOptions(m_options);
        if (m_env)
            m_api->ReleaseEnv(m_env);
    }
    std::pair<int, double> best(const float *logits, int n) const {
        const int index = int(std::max_element(logits, logits + n) - logits);
        double total = 0;
        for (int i = 0; i < n; ++i)
            total += std::exp(double(logits[i] - logits[index]));
        return {index, 1.0 / total};
    }

  public:
    explicit NativeNlu(const QString &path)
        : m_tokenizer(QDir(path).filePath("vocab.txt")), m_labels(json(QDir(path).filePath("labels.json"))),
          m_name(QDir(path).dirName()) {
        if (!m_api)
            throw std::runtime_error("ONNX Runtime API unavailable");
        m_threshold = json(QDir(path).filePath("calibration.json"))["threshold"].toDouble(-1);
        if (m_threshold < 0 || m_labels["max_length"].toInt() != 64 || m_labels["acts"].toArray().isEmpty() ||
            m_labels["intents"].toArray().isEmpty() || m_labels["tags"].toArray().isEmpty())
            throw std::runtime_error("Unsupported NLU schema");
        try {
            check(m_api->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "cockpit-nlu", &m_env));
            check(m_api->CreateSessionOptions(&m_options));
            check(m_api->SetIntraOpNumThreads(m_options, 2));
            check(m_api->SetInterOpNumThreads(m_options, 1));
            const auto file = nluModelPath(path);
            if (file.isEmpty())
                throw std::runtime_error("Invalid NLU inference descriptor");
            const auto model = file.toStdWString();
            check(m_api->CreateSession(m_env, model.c_str(), m_options, &m_session));
            check(m_api->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &m_memory));
        } catch (...) {
            release();
            throw;
        }
    }
    ~NativeNlu() override { release(); }
    QJsonObject run(const QJsonObject &request, const std::atomic_bool &cancelled) override {
        const auto text = request["text"].toString();
        const auto tokens = m_tokenizer.encode(text);
        QJsonObject result{{"text", text},
                           {"model", m_name},
                           {"proposal", QJsonValue::Null},
                           {"block_reason", QJsonValue::Null},
                           {"entities", QJsonArray{}}};
        if (tokens.size() > 64) {
            result["block_reason"] = "too many tokens";
            return result;
        }
        if (cancelled)
            return result;
        std::vector<int64_t> ids, mask(tokens.size(), 1), types(tokens.size(), 0);
        for (const auto &token : tokens)
            ids.push_back(token.id);
        const int64_t shape[]{1, int64_t(tokens.size())};
        OrtValue *inputs[3]{}, *outputs[3]{};
        auto cleanup = qScopeGuard([&] {
            for (auto *v : inputs)
                if (v)
                    m_api->ReleaseValue(v);
            for (auto *v : outputs)
                if (v)
                    m_api->ReleaseValue(v);
        });
        std::vector<int64_t> *data[]{&ids, &mask, &types};
        for (int i = 0; i < 3; ++i)
            check(m_api->CreateTensorWithDataAsOrtValue(m_memory, data[i]->data(), data[i]->size() * 8, shape,
                                                        2, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64, &inputs[i]));
        const char *inputNames[]{"input_ids", "attention_mask", "token_type_ids"};
        const char *outputNames[]{"speech_act_logits", "intent_logits", "slot_logits"};
        check(m_api->Run(m_session, nullptr, inputNames, inputs, 3, outputNames, 3, outputs));
        float *logits[3]{};
        const auto acts = m_labels["acts"].toArray(), intents = m_labels["intents"].toArray(),
                   tags = m_labels["tags"].toArray();
        const size_t sizes[]{size_t(acts.size()), size_t(intents.size()), tokens.size() * tags.size()};
        for (int i = 0; i < 3; ++i) {
            OrtTensorTypeAndShapeInfo *info = nullptr;
            check(m_api->GetTensorTypeAndShape(outputs[i], &info));
            size_t count = 0;
            auto status = m_api->GetTensorShapeElementCount(info, &count);
            m_api->ReleaseTensorTypeAndShapeInfo(info);
            check(status);
            if (count != sizes[i])
                throw std::runtime_error("NLU output dimensions do not match labels");
            check(m_api->GetTensorMutableData(outputs[i], reinterpret_cast<void **>(&logits[i])));
        }
        const auto act = best(logits[0], acts.size()), intent = best(logits[1], intents.size());
        double confidence = std::min(act.second, intent.second);
        result["speech_act"] = acts[act.first];
        result["intent"] = intents[intent.first];
        std::vector<int> slotIds;
        std::vector<std::vector<double>> scores;
        for (size_t i = 0; i < tokens.size(); ++i) {
            const auto *row = logits[2] + i * tags.size();
            const auto selected = best(row, tags.size());
            slotIds.push_back(selected.first);
            std::vector<double> probabilities(tags.size());
            for (int j = 0; j < tags.size(); ++j)
                probabilities[j] = std::exp(double(row[j] - row[selected.first])) * selected.second;
            scores.push_back(std::move(probabilities));
            if (tokens[i].start != tokens[i].end)
                confidence = std::min(confidence, selected.second);
        }
        result["confidence"] = confidence;
        try {
            QJsonArray entities;
            try {
                entities = decodeBio(text, tokens, tags, slotIds);
            } catch (const std::exception &) {
                if (acts[act.first] != "EXECUTE" || intents[intent.first] != "TEMPERATURE_SET")
                    throw;
                slotIds = recoverTemperatureBio(text, tokens, tags, slotIds, scores);
                entities = decodeBio(text, tokens, tags, slotIds);
                for (size_t i = 0; i < tokens.size(); ++i)
                    if (tokens[i].start != tokens[i].end)
                        confidence = std::min(confidence, scores[i][slotIds[i]]);
                result["confidence"] = confidence;
                result["slot_recovery"] = "temperature_bio";
            }
            result["entities"] = entities;
            if (acts[act.first] == "EXECUTE" || acts[act.first] == "QUERY") {
                const auto proposal =
                    normalizeSlots(text, request["input"].toString(), acts[act.first].toString(),
                                   intents[intent.first].toString(), entities);
                if (confidence < m_threshold)
                    throw std::runtime_error("confidence below dev-calibrated threshold");
                result["proposal"] = proposal;
            }
        } catch (const std::exception &error) {
            result["block_reason"] = QString::fromUtf8(error.what());
        }
        return result;
    }
};
std::unique_ptr<InferenceEngine> makeNlu(const QString &path) { return std::make_unique<NativeNlu>(path); }
} // namespace cockpit
