#pragma once
#include <QHash>
#include <QString>
#include <cstdint>
#include <vector>
namespace cockpit {
struct BertToken {
    int64_t id;
    int start;
    int end;
};
class BertTokenizer {
  public:
    explicit BertTokenizer(const QString &vocab);
    std::vector<BertToken> encode(const QString &) const;

  private:
    QHash<QString, int64_t> m_vocab;
};
} // namespace cockpit
