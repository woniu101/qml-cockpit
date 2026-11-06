#pragma once
#include <QJsonObject>
#include <QString>
#include <atomic>
#include <memory>

namespace cockpit {
enum class InferenceKind { Asr, Wake, Nlu, Voice };
class InferenceEngine {
  public:
    virtual ~InferenceEngine() = default;
    virtual QJsonObject run(const QJsonObject &, const std::atomic_bool &) = 0;
};
std::unique_ptr<InferenceEngine> makeAsr(const QString &);
std::unique_ptr<InferenceEngine> makeWake(const QString &);
std::unique_ptr<InferenceEngine> makeNlu(const QString &);
std::unique_ptr<InferenceEngine> makeVoice(const QString &);
QString modelDirectory(InferenceKind kind, bool candidate = false);
QString modelResource(const QString &name);
QJsonObject modelConfiguration();
void verifyBundle(const QString &directory);
QString nluModelPath(const QString &directory);
} // namespace cockpit
