#include "AssistantBatch.h"
#include "VehicleState.h"

#include <QPointer>
#include <QRegularExpression>
#include <QTimer>
#include <utility>

AssistantBatch::AssistantBatch(VehicleState &state, AssistantVehicle &vehicle) : m_state(state), m_vehicle(vehicle) {}

std::optional<QList<AssistantVehicleSetting>> AssistantBatch::parse(const QString &text, bool spoken) {
    static const QRegularExpression separator(QStringLiteral("[，,；;]?\\s*然后\\s*"));
    const auto clauses = text.split(separator);
    if (clauses.size() < 2)
        return std::nullopt;
    QList<AssistantVehicleSetting> result;
    for (qsizetype index = 0; index < clauses.size(); ++index) {
        auto clause = clauses.at(index).trimmed();
        if (index == 0 && clause.startsWith(QStringLiteral("先")))
            clause.remove(0, 1);
        auto setting = assistantVehicleSetting(clause, spoken);
        if (!setting)
            setting = assistantWindowPilotEvidence(clause, spoken);
        if (setting && setting->target >= 0 && setting->rangeError().isEmpty())
            result.append(*setting);
    }
    if (result.isEmpty())
        return std::nullopt;
    return result.size() == clauses.size() && result.size() <= 3 ? result : QList<AssistantVehicleSetting>{};
}

QString AssistantBatch::describe(const AssistantVehicleSetting &setting) {
    return (setting.kind == AssistantVehicleSetting::Kind::Window
        ? QStringLiteral("%1开度 %2%") : QStringLiteral("%1空调 %2 度"))
        .arg(setting.targets().at(setting.target)).arg(setting.value);
}

void AssistantBatch::start(const QList<AssistantVehicleSetting> &settings, AssistantVehicle::Reply reply) {
    if (running() || settings.isEmpty())
        return;
    m_settings = settings;
    m_results.clear();
    for (const auto &setting : settings)
        m_results.append(QVariantMap{{"title", describe(setting)}, {"status", QStringLiteral("等待")}});
    m_source = m_state.sourceMode();
    m_session = m_state.connection()->sessionId();
    m_stopReason.clear();
    m_index = 0;
    m_reply = std::move(reply);
    advance();
}

void AssistantBatch::cancelRemaining() {
    if (running())
        m_stopReason = QStringLiteral("已取消后续指令");
}

void AssistantBatch::clear() {
    if (!running())
        m_results.clear();
}

void AssistantBatch::updateRow(const QString &status, const QString &detail) {
    auto row = m_results.at(m_index).toMap();
    row.insert("status", status);
    row.insert("detail", detail);
    m_results[m_index] = row;
    emit changed();
}

void AssistantBatch::advance() {
    if (m_stopReason.isEmpty() && (!m_state.externalReady() || m_source != m_state.sourceMode()
        || m_session != m_state.connection()->sessionId()))
        m_stopReason = QStringLiteral("车辆数据来源已变化");
    if (m_stopReason.isEmpty() && m_index < m_settings.size() && m_state.climateSync()
        && m_settings.at(m_index).kind == AssistantVehicleSetting::Kind::Temperature)
        m_stopReason = QStringLiteral("空调处于双区同步");
    if (!m_stopReason.isEmpty() || m_index == m_settings.size()) {
        while (m_index < m_settings.size()) {
            updateRow(QStringLiteral("未执行"), m_stopReason);
            ++m_index;
        }
        QStringList details;
        for (const auto &result : m_results) {
            const auto row = result.toMap();
            details.append(row.value("status").toString() == QStringLiteral("未执行")
                ? row.value("title").toString() + QStringLiteral("未执行") : row.value("detail").toString());
        }
        auto reply = std::exchange(m_reply, {});
        reply(details.join(QStringLiteral("。")), true);
        emit changed();
        return;
    }

    m_confirmed = false;
    updateRow(QStringLiteral("执行中"));
    m_reply(QStringLiteral("正在执行第 %1 项，共 %2 项").arg(m_index + 1).arg(m_settings.size()), false);
    m_vehicle.execute(m_settings.at(m_index),
        [self = QPointer<AssistantBatch>(this)](const QString &message, bool final) {
            if (!self || !final)
                return;
            self->updateRow(self->m_confirmed ? QStringLiteral("已确认") : QStringLiteral("未确认"), message);
            if (!self->m_confirmed && self->m_stopReason.isEmpty())
                self->m_stopReason = QStringLiteral("上一项未确认");
            ++self->m_index;
            QTimer::singleShot(0, self, &AssistantBatch::advance);
        }, [self = QPointer<AssistantBatch>(this)] {
            if (self)
                self->m_confirmed = true;
        });
}
