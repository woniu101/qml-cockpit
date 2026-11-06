#pragma once

#include <QString>

class VehicleState;
class QJsonObject;

// Returns an empty string when the request is outside the read-only vehicle vocabulary.
QString assistantVehicleAnswer(const VehicleState &vehicle, const QString &request);
QString assistantBatteryAnswer(const VehicleState &vehicle);
QString assistantWindowAnswer(const VehicleState &vehicle, int selected);
QString assistantModelVehicleAnswer(const VehicleState &vehicle, const QString &request, const QJsonObject &result);
