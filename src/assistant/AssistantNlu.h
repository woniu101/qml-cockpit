#pragma once

#include "AssistantWorker.h"
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
    Q_PROPERTY(int followUpRevision READ followUpRevision NOTIFY changed)
public:
    AssistantNlu(VehicleState &, AssistantVehicle &, AssistantMedia &, int followUpTimeoutMs = 25000);
    QString diagnostic() const { return m_diagnostic; }
    bool busy() const { return m_active && !m_superseded; }
    bool canCorrect() const { return m_last.contains("route"); }
    QVariantMap review() const { return m_last.toVariantMap(); }
    Q_INVOKABLE void submit(const QString &text, bool spoken = false);
    Q_INVOKABLE bool correct(const QString &original, const QString &act, const QString &intent,
                             const QString &parametersJson, const QString &trainingText);
    QStringList followUpOptions() const;
    int followUpRevision() const { return m_followUpRevision; }
    Q_INVOKABLE void chooseFollowUp(int index, int revision);
    Q_INVOKABLE void cancelFollowUp();
    Q_INVOKABLE void endConversation();

signals:
    void changed();

private:
    void complete(const QJsonObject &result);
    void beginReview(const QString &request, bool spoken, const QJsonObject &context = {});
    void submitCandidate();
    void finishReview(const QString &route);
    void recordReview();
    bool dispatchRules(const QString &request, bool spoken);
    bool dispatchSetting(const AssistantVehicleSetting &setting, bool spoken);
    void executeSetting(const AssistantVehicleSetting &setting);
    void clearContext();
    bool sourceMatches(const QString &source, const QString &session) const;
    void finishFollowUp(int target, bool spoken);
    void clearFollowUp();
    bool append(const QJsonObject &record);
    VehicleState &m_state;
    AssistantVehicle &m_vehicle;
    AssistantMedia &m_media;
    AssistantWorker m_worker;
    AssistantWorker m_candidate;
    QTimer m_followUpTimeout;
    QTimer m_contextTimeout;
    std::optional<AssistantVehicleSetting> m_followUp;
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
    QString m_diagnostic, m_text;
    bool m_active = false, m_spoken = false, m_superseded = false, m_recordingSamples = false;
};
