#pragma once
#include <QJsonArray>
#include <QJsonObject>
namespace cockpit {
QJsonObject normalizeSlots(const QString &text, const QString &input, const QString &act,
                           const QString &intent, const QJsonArray &entities);
}
