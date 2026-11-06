#pragma once

#include "AssistantVehicle.h"
#include "AssistantVehicleSetting.h"
#include <QObject>
#include <QVariantList>

class VehicleState;

class AssistantBatch final : public QObject {
    Q_OBJECT
public:
    AssistantBatch(VehicleState &state, AssistantVehicle &vehicle);
    static std::optional<QList<AssistantVehicleSetting>> parse(const QString &text, bool spoken);
    static QString describe(const AssistantVehicleSetting &setting);
    bool running() const { return bool(m_reply); }
    QVariantList results() const { return m_results; }
    void start(const QList<AssistantVehicleSetting> &settings, AssistantVehicle::Reply reply);
    void cancelRemaining();
    void clear();

signals:
    void changed();

private:
    void advance();
    void updateRow(const QString &status, const QString &detail = {});
    VehicleState &m_state;
    AssistantVehicle &m_vehicle;
    QList<AssistantVehicleSetting> m_settings;
    QVariantList m_results;
    QString m_source, m_session, m_stopReason;
    AssistantVehicle::Reply m_reply;
    qsizetype m_index = 0;
    bool m_confirmed = false;
};
