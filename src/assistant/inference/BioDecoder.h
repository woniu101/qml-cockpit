#pragma once
#include "BertTokenizer.h"
#include <QJsonArray>
namespace cockpit {
QJsonArray decodeBio(const QString &, const std::vector<BertToken> &, const QJsonArray &,
                     const std::vector<int> &);
std::vector<int> recoverTemperatureBio(const QString &, const std::vector<BertToken> &, const QJsonArray &,
                                       const std::vector<int> &, const std::vector<std::vector<double>> &);
} // namespace cockpit
