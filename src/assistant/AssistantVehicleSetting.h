#pragma once

#include <QStringList>
#include <optional>

struct AssistantVehicleSetting {
    enum class Kind { Temperature, Window };
    Kind kind;
    int target;
    double value;
    bool relative = false;

    QStringList targets() const;
    QString prompt() const;
    QString rangeError() const;
    int targetFromReply(const QString &text) const;
};

struct AssistantRetarget {
    AssistantVehicleSetting::Kind kind;
    int target;
};

std::optional<AssistantVehicleSetting> assistantVehicleSetting(const QString &request, bool spoken);
std::optional<AssistantVehicleSetting> assistantWindowPilotEvidence(const QString &request, bool spoken);
std::optional<double> assistantSettingValue(AssistantVehicleSetting::Kind kind, const QString &text, bool spoken);
std::optional<AssistantRetarget> assistantRetarget(const QString &request);
bool assistantHasMultipleVehicleActions(const QString &request);
std::optional<AssistantVehicleSetting> assistantRelativeVehicleSetting(const QString &request);
std::optional<int> assistantWindowCloseTarget(const QString &request);
bool assistantCancelsRequest(const QString &request);
