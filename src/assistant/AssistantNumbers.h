#pragma once

#include <QRegularExpression>

inline int assistantCommandInteger(const QString &text) {
    bool ok = false;
    const int decimal = text.toInt(&ok);
    if (ok)
        return decimal;
    if (text == QStringLiteral("零") || text == QStringLiteral("〇"))
        return 0;
    static const QRegularExpression pattern(QStringLiteral(
        "^(?:([一二两三四五六七八九])百)?(?:([一二两三四五六七八九])?十)?零?([一二两三四五六七八九])?$"));
    const auto parts = pattern.match(text);
    if (text.isEmpty() || !parts.hasMatch())
        return -1;
    auto digit = [](const QString &value) {
        if (value.isEmpty())
            return 0;
        if (value == QStringLiteral("两"))
            return 2;
        return int(QStringLiteral("零一二三四五六七八九").indexOf(value));
    };
    const int tens = text.contains(QChar(u'十')) ? (parts.captured(2).isEmpty() ? 1 : digit(parts.captured(2))) : 0;
    return 100 * digit(parts.captured(1)) + 10 * tens + digit(parts.captured(3));
}
