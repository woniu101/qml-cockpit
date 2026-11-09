#include "VoiceText.h"

#include <QRegularExpression>
#include <QStringView>

namespace cockpit {
namespace {
constexpr QStringView Digits(u"零一二三四五六七八九");

QString readDigits(QStringView digits) {
    QString spoken;
    for (const QChar digit : digits)
        spoken += Digits.at(digit.digitValue());
    return spoken;
}

QString readInteger(QStringView digits) {
    // Leading zeroes and long identifiers are read digit by digit.
    if (digits.size() == 1 || digits.startsWith(u'0') || digits.size() > 4)
        return readDigits(digits);

    constexpr QStringView Places(u" 十百千");
    QString spoken;
    bool gap = false;
    for (qsizetype i = 0; i < digits.size(); ++i) {
        const int digit = digits.at(i).digitValue();
        const auto place = digits.size() - i - 1;
        if (digit == 0) {
            gap = true;
            continue;
        }
        if (gap)
            spoken += u'零';
        if (digit != 1 || place != 1 || i != 0)
            spoken += Digits.at(digit);
        if (place > 0)
            spoken += Places.at(place);
        gap = false;
    }
    return spoken;
}
}

QString voiceTextForSynthesis(const QString &text) {
    static const QRegularExpression number(
        QStringLiteral(R"(((?<![A-Za-z0-9])[+−-])?([0-9]+)(?:\.([0-9]+))?([ \t]*[%％])?)"));
    auto matches = number.globalMatch(text);
    QString spoken;
    qsizetype from = 0;
    while (matches.hasNext()) {
        const auto match = matches.next();
        spoken += QStringView(text).sliced(from, match.capturedStart() - from);
        const auto sign = match.capturedView(1);
        if (!sign.isEmpty())
            spoken += sign.front() == u'+' ? u'正' : u'负';
        if (match.hasCaptured(4))
            spoken += QStringLiteral("百分之");
        spoken += readInteger(match.capturedView(2));
        if (match.hasCaptured(3)) {
            spoken += u'点';
            spoken += readDigits(match.capturedView(3));
        }
        from = match.capturedEnd();
    }
    spoken += QStringView(text).sliced(from);
    return spoken;
}
} // namespace cockpit
