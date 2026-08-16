#include "parser/SpxParser.h"

#include <QFile>
#include <QXmlStreamReader>
#include <limits>

namespace {

QString tokenName(const QXmlStreamReader &xml)
{
    return xml.name().toString();
}

PlistValue parseInteger(const QString &text)
{
    bool ok = false;
    const qint64 signedValue = text.toLongLong(&ok, 10);
    if (ok) {
        return PlistValue::integer(signedValue);
    }

    const qulonglong unsignedValue = text.toULongLong(&ok, 10);
    if (ok && unsignedValue <= static_cast<qulonglong>(std::numeric_limits<qint64>::max())) {
        return PlistValue::integer(static_cast<qint64>(unsignedValue));
    }

    return PlistValue::string(text);
}

PlistValue parseReal(const QString &text)
{
    bool ok = false;
    const double value = text.toDouble(&ok);
    return ok ? PlistValue::real(value) : PlistValue::string(text);
}

} // namespace

SpxParser::Result SpxParser::parseFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        Result result;
        result.error = QStringLiteral("Could not open “%1”: %2")
                           .arg(path, file.errorString());
        return result;
    }

    QByteArray header = file.peek(8);
    if (header.startsWith("bplist")) {
        Result result;
        result.error = QStringLiteral("This looks like a binary property list, which is not supported. "
                                      "Apple System Information reports are XML property lists.");
        return result;
    }

    return parseDevice(&file);
}

SpxParser::Result SpxParser::parseDevice(QIODevice *device)
{
    Result result;
    QXmlStreamReader xml(device);
    xml.setNamespaceProcessing(false);

    PlistValue root;
    bool foundValue = false;

    while (!xml.atEnd()) {
        const QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartElement) {
            const QString name = tokenName(xml);
            if (name == QLatin1String("plist")) {
                continue;
            }
            root = parseValue(xml);
            foundValue = true;
            break;
        }
    }

    if (xml.hasError()) {
        result.error = xml.errorString();
        result.line = static_cast<int>(xml.lineNumber());
        result.column = static_cast<int>(xml.columnNumber());
        return result;
    }

    if (!foundValue || root.isNull()) {
        result.error = QStringLiteral("The file does not contain a property list.");
        return result;
    }

    result.ok = true;
    result.root = std::move(root);
    return result;
}

bool SpxParser::looksLikeSystemReport(const PlistValue &root)
{
    if (!root.isArray() || root.arrayItems().isEmpty()) {
        return false;
    }

    int dataTypeCount = 0;
    for (const PlistValue &item : root.arrayItems()) {
        if (!item.isDictionary()) {
            continue;
        }
        if (item.contains(QStringLiteral("_dataType"))) {
            ++dataTypeCount;
        }
    }
    return dataTypeCount > 0;
}

PlistValue SpxParser::parseValue(QXmlStreamReader &xml)
{
    const QString name = tokenName(xml);

    if (name == QLatin1String("dict")) {
        return parseDict(xml);
    }
    if (name == QLatin1String("array")) {
        return parseArray(xml);
    }
    if (name == QLatin1String("string")) {
        return PlistValue::string(xml.readElementText());
    }
    if (name == QLatin1String("integer")) {
        return parseInteger(xml.readElementText());
    }
    if (name == QLatin1String("real")) {
        return parseReal(xml.readElementText());
    }
    if (name == QLatin1String("true")) {
        xml.skipCurrentElement();
        return PlistValue::boolean(true);
    }
    if (name == QLatin1String("false")) {
        xml.skipCurrentElement();
        return PlistValue::boolean(false);
    }
    if (name == QLatin1String("date")) {
        const QString text = xml.readElementText();
        QDateTime date = QDateTime::fromString(text, Qt::ISODate);
        if (!date.isValid()) {
            date = QDateTime::fromString(text, Qt::ISODateWithMs);
        }
        return date.isValid() ? PlistValue::date(date) : PlistValue::string(text);
    }
    if (name == QLatin1String("data")) {
        const QString text = xml.readElementText().remove(QLatin1Char(' ')).remove(QLatin1Char('\n')).remove(QLatin1Char('\t'));
        return PlistValue::data(QByteArray::fromBase64(text.toLatin1()));
    }

    xml.skipCurrentElement();
    return {};
}

PlistValue SpxParser::parseDict(QXmlStreamReader &xml)
{
    QVector<QPair<QString, PlistValue>> items;
    QString pendingKey;

    while (!xml.atEnd()) {
        const QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::EndElement) {
            break;
        }
        if (token != QXmlStreamReader::StartElement) {
            continue;
        }

        const QString name = tokenName(xml);
        if (name == QLatin1String("key")) {
            pendingKey = xml.readElementText();
            continue;
        }

        PlistValue value = parseValue(xml);
        if (pendingKey.isEmpty()) {
            continue;
        }
        items.append(qMakePair(pendingKey, std::move(value)));
        pendingKey.clear();
    }

    return PlistValue::dictionary(std::move(items));
}

PlistValue SpxParser::parseArray(QXmlStreamReader &xml)
{
    QVector<PlistValue> items;

    while (!xml.atEnd()) {
        const QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::EndElement) {
            break;
        }
        if (token != QXmlStreamReader::StartElement) {
            continue;
        }
        items.append(parseValue(xml));
    }

    return PlistValue::array(std::move(items));
}
