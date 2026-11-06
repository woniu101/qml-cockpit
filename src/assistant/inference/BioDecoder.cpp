#include "BioDecoder.h"
#include <QJsonObject>
#include <QRegularExpression>
#include <stdexcept>
namespace cockpit {
QJsonArray decodeBio(const QString &text, const std::vector<BertToken> &tokens, const QJsonArray &tags,
                     const std::vector<int> &ids) {
    QJsonArray entities;
    QString previous;
    int previousEnd = -1;
    const auto points = text.toUcs4();
    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto &t = tokens[i];
        if (t.start == t.end)
            continue;
        const auto tag = tags[ids[i]].toString();
        if (tag == "O") {
            previous.clear();
            continue;
        }
        const auto label = tag.mid(2);
        if (tag.startsWith("B-"))
            entities.append(
                QJsonObject{{"start", t.start},
                            {"end", t.end},
                            {"label", label},
                            {"text", QString::fromUcs4(points.data() + t.start, t.end - t.start)}});
        else if (tag.startsWith("I-") && previous == label && previousEnd == t.start && !entities.isEmpty()) {
            auto entity = entities.last().toObject();
            entity["end"] = t.end;
            entity["text"] =
                QString::fromUcs4(points.data() + entity["start"].toInt(), t.end - entity["start"].toInt());
            entities[entities.size() - 1] = entity;
        } else
            throw std::runtime_error("invalid BIO transition");
        previous = label;
        previousEnd = t.end;
    }
    return entities;
}
std::vector<int> recoverTemperatureBio(const QString &text, const std::vector<BertToken> &tokens,
                                       const QJsonArray &tags, const std::vector<int> &ids,
                                       const std::vector<std::vector<double>> &scores) {
    auto require = [](bool ok, const char *reason) {
        if (!ok)
            throw std::runtime_error(reason);
    };
    const QRegularExpression zone(QStringLiteral("主驾驶|副驾驶|驾驶位|驾驶员|乘客侧|主驾|副驾|主价|主架"));
    const QRegularExpression numeric(
        QStringLiteral("[-+负正]?(?:百分之)?(?:\\d+(?:\\.\\d+)?|[零一二两三四五六七八九十百]+(?:点["
                       "零一二三四五六七八九]+)?)[%％]?"));
    auto zones = zone.globalMatch(text), values = numeric.globalMatch(text);
    require(zones.hasNext() && values.hasNext(), "temperature repair requires one unambiguous setting");
    const auto z = zones.next(), v = values.next();
    require(!zones.hasNext() && !values.hasNext() &&
                QRegularExpression(QStringLiteral("调到|调至|调成|设为|设到|设成|设置|改成|定下|定为|改到"))
                    .match(text)
                    .hasMatch() &&
                !QRegularExpression(
                     QStringLiteral("不[要用是许]|别|取消|先等|还是|之后|再|或|如果|[，,；;。！？!?](?!$)"))
                     .match(text)
                     .hasMatch(),
            "temperature repair requires one unambiguous setting");
    const auto unit = QRegularExpression(QStringLiteral("^\\s*(摄氏度|度)")).match(text.mid(v.capturedEnd()));
    require(unit.hasMatch(), "temperature repair requires an explicit degree unit");
    auto cp = [&](int offset) { return int(text.left(offset).toUcs4().size()); };
    struct Span {
        QString label;
        int start, end, covered;
    };
    std::vector<Span> spans{{"ZONE", cp(z.capturedStart()), cp(z.capturedEnd()), cp(z.capturedStart())},
                            {"VALUE", cp(v.capturedStart()), cp(v.capturedEnd()), cp(v.capturedStart())},
                            {"UNIT", cp(v.capturedEnd() + unit.capturedStart(1)),
                             cp(v.capturedEnd() + unit.capturedEnd(1)),
                             cp(v.capturedEnd() + unit.capturedStart(1))}};
    auto repaired = ids;
    int corrections = 0, previousEnd = -1;
    QString previous = "O";
    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto &t = tokens[i];
        if (t.start == t.end)
            continue;
        const auto tag = tags[ids[i]].toString();
        const bool illegal =
            tag.startsWith("I-") && (previous.mid(2) != tag.mid(2) || previousEnd != t.start);
        QString expected = "O";
        for (auto &span : spans) {
            if (t.start >= span.end || t.end <= span.start)
                continue;
            require(t.start == span.covered && t.end <= span.end, "temperature span splits a token");
            expected = (t.start == span.start ? "B-" : "I-") + span.label;
            span.covered = t.end;
            break;
        }
        int selected = -1;
        for (int j = 0; j < tags.size(); ++j)
            if (tags[j].toString() == expected)
                selected = j;
        require(selected >= 0, "unsupported BIO label");
        if (tag != expected) {
            require(illegal && (expected == "O" || expected == "B-" + tag.mid(2)),
                    "temperature repair would change a valid slot");
            require(scores[i][selected] >= 0.03, "temperature repair lacks token evidence");
            ++corrections;
        }
        require(expected == "O" || scores[i][selected] >= 0.75, "temperature repair lacks slot evidence");
        repaired[i] = selected;
        previous = tag;
        previousEnd = t.end;
    }
    require(corrections >= 1 && corrections <= 2, "temperature repair incomplete");
    for (const auto &span : spans)
        require(span.covered == span.end, "temperature repair incomplete");
    return repaired;
}
} // namespace cockpit
