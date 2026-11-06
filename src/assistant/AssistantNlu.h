#pragma once

#include "AssistantInference.h"
#include "AssistantBatch.h"
#include "AssistantVehicleSetting.h"
#include <QVariantMap>

class AssistantMedia;
class AssistantVehicle;
class VehicleState;

class AssistantNlu final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString diagnostic READ diagnostic NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool recordingSamples MEMBER m_recordingSamples NOTIFY changed)
    Q_PROPERTY(bool canCorrect READ canCorrect NOTIFY changed)
    Q_PROPERTY(QVariantMap review READ review NOTIFY changed)
    Q_PROPERTY(QStringList followUpOptions READ followUpOptions NOTIFY changed)
    Q_PROPERTY(bool followUpActive READ followUpActive NOTIFY changed)
    Q_PROPERTY(QString followUpHint READ followUpHint NOTIFY changed)
    Q_PROPERTY(int followUpRevision READ followUpRevision NOTIFY changed)
    Q_PROPERTY(QVariantList batchResults READ batchResults NOTIFY changed)
    Q_PROPERTY(bool batchRunning READ batchRunning NOTIFY changed)
public:
    AssistantNlu(VehicleState &, AssistantVehicle &, AssistantMedia &, int followUpTimeoutMs = 25000);
    QString diagnostic() const { return m_diagnostic; }
    bool busy() const { return (m_active && !m_superseded) || m_windowPilot.has_value() || batchRunning(); }
    bool batchRunning() const { return m_batch.running(); }
    QVariantList batchResults() const { return m_batch.results(); }
    Q_INVOKABLE void cancelRemaining() { m_batch.cancelRemaining(); }
    bool canCorrect() const { return m_last.contains("route"); }
    QVariantMap review() const { return m_last.toVariantMap(); }
    Q_INVOKABLE void submit(const QString &text, bool spoken = false);
    Q_INVOKABLE bool correct(const QString &original, const QString &act, const QString &intent,
                             const QString &parametersJson, const QString &trainingText);
    QStringList followUpOptions() const;
    bool followUpActive() const { return m_followUp.has_value(); }
    QString followUpHint() const;
    int followUpRevision() const { return m_followUpRevision; }
    Q_INVOKABLE void chooseFollowUp(int index, int revision);
    Q_INVOKABLE void cancelFollowUp();
    Q_INVOKABLE void endConversation();
    void setPromptActive(bool active);

signals:
    void changed();

private:
    enum class FollowUpKind { Target, Value, ConfirmTarget, WindowQuery, BatteryQuery, Batch };
    struct FollowUp {
        FollowUpKind kind;
        AssistantVehicleSetting setting{AssistantVehicleSetting::Kind::Temperature, -1, 0};
        bool climateSync = false;
        bool modelWindow = false;
        QList<AssistantVehicleSetting> batch;
    };
    void complete(const QJsonObject &result);
    void beginReview(const QString &request, bool spoken, const QJsonObject &context = {});
    void submitCandidate();
    void finishReview(const QString &route);
    void recordReview();
    bool dispatchRules(const QString &request, bool spoken);
    bool dispatchSetting(const AssistantVehicleSetting &setting, bool spoken);
    bool handleDialogue(const QString &request, bool spoken);
    bool handleFollowUp(const QString &request, bool spoken);
    void startFollowUp(const FollowUp &followUp, const QString &prompt, bool spoken);
    void executeSetting(const AssistantVehicleSetting &setting, bool modelWindow = false);
    bool beginWindowPilot(const QString &request, bool spoken);
    void finishWindowPilot(const QJsonObject &result);
    void markWindowPilot(const QString &status, const QString &message = {});
    void clearContext();
    bool sourceMatches(const QString &source, const QString &session) const;
    void finishFollowUp(int target, bool spoken);
    void clearFollowUp();
    bool append(const QJsonObject &record);
    VehicleState &m_state;
    AssistantVehicle &m_vehicle;
    AssistantMedia &m_media;
    AssistantBatch m_batch;
    AssistantInference m_worker;
    AssistantInference m_candidate;
    QTimer m_followUpTimeout;
    QTimer m_contextTimeout;
    std::optional<FollowUp> m_followUp;
    std::optional<AssistantVehicleSetting> m_windowPilot;
    QString m_windowPilotSource, m_windowPilotSession;
    bool m_windowPilotSpoken = false;
    const bool m_windowPilotEnabled = qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS")
        ? qEnvironmentVariableIntValue("CAR_TERMINAL_NLU_WINDOW_PILOT") == 1
        : cockpit::modelConfiguration().value("nluWindowPilot").toBool();
    const QString m_candidateModelName = cockpit::modelConfiguration().value("nluCandidate").toString();
    std::optional<AssistantVehicleSetting> m_lastSetting;
    QString m_followUpSource, m_followUpSession;
    QString m_contextSource, m_contextSession;
    QString m_contextText;
    double m_contextValue = 0;
    quint64 m_contextRevision = 0;
    int m_followUpRevision = 0;
    bool m_followUpSpoken = false;
    QJsonObject m_last;
    quint64 m_reviewRevision = 0, m_candidateRevision = 0;
    bool m_candidatePending = false, m_reviewSaved = false;
    bool m_promptActive = false;
    QString m_diagnostic, m_text;
    bool m_active = false, m_spoken = false, m_superseded = false, m_recordingSamples = false;
};
