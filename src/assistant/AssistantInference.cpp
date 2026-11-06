#include "AssistantInference.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>

namespace cockpit {
QString nluModelPath(const QString &directory) {
    QFile descriptor(QDir(directory).filePath("inference.json"));
    if (!descriptor.open(QIODevice::ReadOnly))
        return {};
    const auto config = QJsonDocument::fromJson(descriptor.readAll()).object();
    const auto file = config["file"].toString();
    if (config["schema"].toString() != "cockpit-nlu-engine-v1" ||
        !QStringList{"fp32", "int8"}.contains(config["precision"].toString()) ||
        QFileInfo(file).fileName() != file || !file.endsWith(".onnx") || file.contains(':'))
        return {};
    return QDir(directory).filePath(file);
}
QJsonObject modelConfiguration() {
    const auto manifest = qEnvironmentVariable(
        "COCKPIT_ASSISTANT_MANIFEST", QCoreApplication::applicationDirPath() + "/assistant/models.json");
    QFile file(manifest);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const auto json = QJsonDocument::fromJson(file.readAll()).object();
    return json["schema"].toString() == "cockpit-native-models-v1" ? json : QJsonObject{};
}
QString modelResource(const QString &name) {
    const auto manifest = qEnvironmentVariable(
        "COCKPIT_ASSISTANT_MANIFEST", QCoreApplication::applicationDirPath() + "/assistant/models.json");
    const auto json = modelConfiguration();
    const auto relative = json.value(name).toString();
    return relative.isEmpty() ? QString()
                              : QDir(QFileInfo(manifest).absolutePath()).absoluteFilePath(relative);
}
QString modelDirectory(InferenceKind kind, bool candidate) {
    const QString key = candidate ? "nluCandidate" : QStringList{"asr", "wake", "nlu", "voice"}.at(int(kind));
    const QString env = candidate ? "CAR_TERMINAL_NLU_CANDIDATE"
                                  : QStringList{"CAR_TERMINAL_ASR_MODEL", "CAR_TERMINAL_KWS_MODEL",
                                                "CAR_TERMINAL_NLU_MODEL", "CAR_TERMINAL_TTS_MODEL"}
                                        .at(int(kind));
    if (qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS") &&
        !qEnvironmentVariable(env.toUtf8()).isEmpty())
        return qEnvironmentVariable(env.toUtf8());
    return modelResource(key);
}
void verifyBundle(const QString &directory) {
    QFile file(QDir(directory).filePath("bundle.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        if (qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS"))
            return;
        throw std::runtime_error("Missing native model bundle manifest");
    }
    const auto bundle = QJsonDocument::fromJson(file.readAll()).object();
    if (bundle["schema"].toString() != "cockpit-native-bundle-v1")
        throw std::runtime_error("Unsupported model bundle");
    const auto files = bundle["files"].toObject();
    if (files.isEmpty())
        throw std::runtime_error("Empty model bundle");
    const QDir root(directory);
    for (auto it = files.begin(); it != files.end(); ++it) {
        const auto path = QDir::cleanPath(root.absoluteFilePath(it.key()));
        if (!path.startsWith(root.absolutePath() + "/", Qt::CaseInsensitive))
            throw std::runtime_error("Invalid model bundle path");
        QFile asset(path);
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (!asset.open(QIODevice::ReadOnly) || !hash.addData(&asset) ||
            hash.result().toHex() != it.value().toString().toLatin1())
            throw std::runtime_error("Model bundle hash mismatch");
    }
}
} // namespace cockpit

AssistantInference::AssistantInference(cockpit::InferenceKind kind, QString model)
    : m_kind(kind), m_model(std::move(model)), m_context(new QObject) {
    m_context->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_context, &QObject::deleteLater);
    m_thread.start();
}
AssistantInference::~AssistantInference() {
    cancel();
    QMetaObject::invokeMethod(m_context, [this] { m_engine.reset(); }, Qt::BlockingQueuedConnection);
    m_thread.quit();
    m_thread.wait();
}
bool AssistantInference::configured() const {
    if (m_model.isEmpty())
        return false;
    if (m_kind == cockpit::InferenceKind::Nlu)
        return QFileInfo(cockpit::nluModelPath(m_model)).isFile();
    const auto name =
        QStringList{"model.int8.onnx", "tokens.txt", "cockpit-nlu.int8.onnx", "encoder.int8.onnx"}.at(
            int(m_kind));
    return QFileInfo(QDir(m_model).filePath(name)).isFile();
}
void AssistantInference::cancel() {
    if (m_cancel)
        *m_cancel = true;
    m_busy = false;
}
bool AssistantInference::submit(const QJsonObject &request) {
    if (m_busy || !configured())
        return false;
    m_busy = true;
    const auto cancelled = m_cancel = std::make_shared<std::atomic_bool>(false);
    QMetaObject::invokeMethod(
        m_context,
        [this, request, cancelled] {
            QElapsedTimer timer;
            timer.start();
            QJsonObject result;
            try {
                if (!m_engine) {
                    cockpit::verifyBundle(m_model);
                    switch (m_kind) {
                    case cockpit::InferenceKind::Asr:
                        m_engine = cockpit::makeAsr(m_model);
                        break;
                    case cockpit::InferenceKind::Wake:
                        m_engine = cockpit::makeWake(m_model);
                        break;
                    case cockpit::InferenceKind::Nlu:
                        m_engine = cockpit::makeNlu(m_model);
                        break;
                    case cockpit::InferenceKind::Voice:
                        m_engine = cockpit::makeVoice(m_model);
                        break;
                    }
                }
                if (!*cancelled)
                    result = m_engine->run(request, *cancelled);
            } catch (const std::exception &error) {
                result = {{"error", QString::fromUtf8(error.what())}};
            }
            result.insert("elapsed_ms", timer.elapsed());
            QMetaObject::invokeMethod(
                this,
                [this, result, cancelled] {
                    if (*cancelled)
                        return;
                    m_busy = false;
                    emit completed(result);
                },
                Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
    return true;
}
