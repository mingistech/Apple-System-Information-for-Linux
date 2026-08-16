#pragma once

#include <QString>

class PlistValue;
struct PropertyInfo;

class LabelHumanizer
{
public:
    static QString dataTypeName(const QString &dataType);
    static QString keyName(const QString &key);
    static QString displayValue(const QString &key, const PlistValue &value, const PropertyInfo *info = nullptr);
    static QString displayText(const QString &key, const QString &rawValue);
    static QString formatByteSize(qint64 bytes);
    static QString itemTitle(const PlistValue &item);

private:
    static QString splitIdentifier(const QString &text);
    static QString titleCase(const QString &text);
    static QString stripKnownPrefix(const QString &key);
    static bool looksLikeIdentifier(const QString &text);
};
