#pragma once
#include "inference/InferenceEngine.h"
#include <QObject>
#include <QThread>
#include <memory>

class AssistantInference final : public QObject {
    Q_OBJECT
  public:
    AssistantInference(cockpit::InferenceKind kind, QString model);
    ~AssistantInference() override;
    bool configured() const;
    bool submit(const QJsonObject &);
    void cancel();
  signals:
    void completed(const QJsonObject &);

  private:
    cockpit::InferenceKind m_kind;
    QString m_model;
    QThread m_thread;
    QObject *m_context;
    std::unique_ptr<cockpit::InferenceEngine> m_engine;
    std::shared_ptr<std::atomic_bool> m_cancel;
    bool m_busy = false;
};
