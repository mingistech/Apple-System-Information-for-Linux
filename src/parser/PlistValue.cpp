#include "parser/PlistValue.h"

namespace {

const QVector<PlistValue> kEmptyArray;
const QVector<QPair<QString, PlistValue>> kEmptyDict;

bool containsQuery(const QString &text, const QString &query)
{
    return !query.isEmpty() && text.contains(query, Qt::CaseInsensitive);
}

} // namespace

class PlistValue::Data : public QSharedData
{
public:
    Type type = Type::Null;
    QString string;
    qint64 integer = 0;
    double real = 0.0;
    bool boolean = false;
    QDateTime date;
    QByteArray data;
    QVector<PlistValue> array;
    QVector<QPair<QString, PlistValue>> dict;
};

PlistValue::PlistValue()
    : d(new Data)
{
}

PlistValue::PlistValue(const PlistValue &other) = default;
PlistValue::PlistValue(PlistValue &&other) noexcept = default;
PlistValue &PlistValue::operator=(const PlistValue &other) = default;
PlistValue &PlistValue::operator=(PlistValue &&other) noexcept = default;
PlistValue::~PlistValue() = default;

PlistValue PlistValue::string(QString value)
{
    PlistValue result;
    result.d->type = Type::String;
    result.d->string = std::move(value);
    return result;
}

PlistValue PlistValue::integer(qint64 value)
{
    PlistValue result;
    result.d->type = Type::Integer;
    result.d->integer = value;
    return result;
}

PlistValue PlistValue::real(double value)
{
    PlistValue result;
    result.d->type = Type::Real;
    result.d->real = value;
    return result;
}

PlistValue PlistValue::boolean(bool value)
{
    PlistValue result;
    result.d->type = Type::Boolean;
    result.d->boolean = value;
    return result;
}

PlistValue PlistValue::date(QDateTime value)
{
    PlistValue result;
    result.d->type = Type::Date;
    result.d->date = std::move(value);
    return result;
}

PlistValue PlistValue::data(QByteArray value)
{
    PlistValue result;
    result.d->type = Type::Data;
    result.d->data = std::move(value);
    return result;
}

PlistValue PlistValue::array(QVector<PlistValue> items)
{
    PlistValue result;
    result.d->type = Type::Array;
    result.d->array = std::move(items);
    return result;
}

PlistValue PlistValue::dictionary(QVector<QPair<QString, PlistValue>> items)
{
    PlistValue result;
    result.d->type = Type::Dictionary;
    result.d->dict = std::move(items);
    return result;
}

PlistValue::Type PlistValue::type() const
{
    return d->type;
}

bool PlistValue::isNull() const { return d->type == Type::Null; }
bool PlistValue::isString() const { return d->type == Type::String; }
bool PlistValue::isInteger() const { return d->type == Type::Integer; }
bool PlistValue::isReal() const { return d->type == Type::Real; }
bool PlistValue::isBoolean() const { return d->type == Type::Boolean; }
bool PlistValue::isDate() const { return d->type == Type::Date; }
bool PlistValue::isData() const { return d->type == Type::Data; }
bool PlistValue::isArray() const { return d->type == Type::Array; }
bool PlistValue::isDictionary() const { return d->type == Type::Dictionary; }

QString PlistValue::toString() const
{
    return d->string;
}

qint64 PlistValue::toInteger() const
{
    return d->integer;
}

double PlistValue::toReal() const
{
    return d->real;
}

bool PlistValue::toBoolean() const
{
    return d->boolean;
}

QDateTime PlistValue::toDate() const
{
    return d->date;
}

QByteArray PlistValue::toData() const
{
    return d->data;
}

const QVector<PlistValue> &PlistValue::arrayItems() const
{
    return isArray() ? d->array : kEmptyArray;
}

const QVector<QPair<QString, PlistValue>> &PlistValue::dictItems() const
{
    return isDictionary() ? d->dict : kEmptyDict;
}

const PlistValue *PlistValue::child(const QString &key) const
{
    if (!isDictionary()) {
        return nullptr;
    }
    for (const auto &entry : d->dict) {
        if (entry.first == key) {
            return &entry.second;
        }
    }
    return nullptr;
}

QString PlistValue::childString(const QString &key) const
{
    const PlistValue *value = child(key);
    return (value && value->isString()) ? value->toString() : QString();
}

bool PlistValue::contains(const QString &key) const
{
    return child(key) != nullptr;
}

bool PlistValue::matches(const QString &query) const
{
    if (query.isEmpty()) {
        return true;
    }

    switch (d->type) {
    case Type::String:
        return containsQuery(d->string, query);
    case Type::Integer:
        return containsQuery(QString::number(d->integer), query);
    case Type::Real:
        return containsQuery(QString::number(d->real, 'g', 15), query);
    case Type::Boolean:
        return containsQuery(d->boolean ? QStringLiteral("true") : QStringLiteral("false"), query)
            || containsQuery(d->boolean ? QStringLiteral("yes") : QStringLiteral("no"), query);
    case Type::Date:
        return containsQuery(d->date.toString(Qt::ISODate), query)
            || containsQuery(d->date.toString(Qt::TextDate), query);
    case Type::Data:
        return containsQuery(QString::fromLatin1(d->data.toHex()), query);
    case Type::Array:
        for (const PlistValue &item : d->array) {
            if (item.matches(query)) {
                return true;
            }
        }
        return false;
    case Type::Dictionary:
        for (const auto &entry : d->dict) {
            if (containsQuery(entry.first, query) || entry.second.matches(query)) {
                return true;
            }
        }
        return false;
    case Type::Null:
        return false;
    }
    return false;
}
