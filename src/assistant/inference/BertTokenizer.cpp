#include "BertTokenizer.h"
#include <QFile>
#include <stdexcept>
namespace cockpit {
BertTokenizer::BertTokenizer(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Cannot read BERT vocab");
    int index = 0;
    for (auto word : QString::fromUtf8(file.readAll()).split('\n')) {
        word.remove('\r');
        if (!word.isEmpty())
            m_vocab.insert(word, index);
        ++index;
    }
    for (const auto &special : {"[CLS]", "[SEP]", "[UNK]", "[PAD]"})
        if (!m_vocab.contains(special))
            throw std::runtime_error("Invalid BERT vocab");
}
std::vector<BertToken> BertTokenizer::encode(const QString &text) const {
    std::vector<BertToken> result{{m_vocab["[CLS]"], 0, 0}};
    QString word;
    std::vector<std::pair<int, int>> offsets;
    auto flush = [&] {
        if (word.isEmpty())
            return;
        std::vector<BertToken> pieces;
        int pos = 0;
        while (pos < word.size() && word.size() <= 100) {
            int end = word.size();
            QString piece;
            for (; end > pos; --end) {
                piece = (pos ? QStringLiteral("##") : QString()) + word.mid(pos, end - pos);
                if (m_vocab.contains(piece))
                    break;
            }
            if (end == pos)
                break;
            pieces.push_back({m_vocab[piece], offsets[pos].first, offsets[end - 1].second});
            pos = end;
        }
        if (pos != word.size())
            result.push_back({m_vocab["[UNK]"], offsets.front().first, offsets.back().second});
        else
            result.insert(result.end(), pieces.begin(), pieces.end());
        word.clear();
        offsets.clear();
    };
    const auto points = text.toUcs4();
    for (int i = 0; i < points.size(); ++i) {
        const auto cp = points[i];
        if (cp == '[') {
            bool special = false;
            for (const auto &name :
                 {QStringLiteral("[CLS]"), QStringLiteral("[SEP]"), QStringLiteral("[PAD]"),
                  QStringLiteral("[MASK]"), QStringLiteral("[UNK]")}) {
                if (i + name.size() <= points.size() &&
                    QString::fromUcs4(points.data() + i, name.size()) == name) {
                    flush();
                    result.push_back({m_vocab[name], i, int(i + name.size())});
                    i += name.size() - 1;
                    special = true;
                    break;
                }
            }
            if (special)
                continue;
        }
        if (QChar::isSpace(cp)) {
            flush();
            continue;
        }
        const auto category = QChar::category(cp);
        if (cp == 0 || cp == 0xfffd || category == QChar::Other_Control || category == QChar::Other_Format)
            continue;
        const bool separate =
            QChar::script(cp) == QChar::Script_Han || (cp >= 33 && cp <= 47) || (cp >= 58 && cp <= 64) ||
            (cp >= 91 && cp <= 96) || (cp >= 123 && cp <= 126) ||
            (category >= QChar::Punctuation_Connector && category <= QChar::Punctuation_Other);
        if (separate)
            flush();
        auto normalized = QString::fromUcs4(&cp, 1).toLower().normalized(QString::NormalizationForm_D);
        for (const auto ch : normalized)
            if (ch.category() != QChar::Mark_NonSpacing) {
                word += ch;
                offsets.emplace_back(i, i + 1);
            }
        if (separate)
            flush();
    }
    flush();
    result.push_back({m_vocab["[SEP]"], 0, 0});
    return result;
}
} // namespace cockpit
