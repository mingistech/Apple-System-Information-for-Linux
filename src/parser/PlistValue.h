#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QPair>
#include <QSharedDataPointer>
#include <QString>
#include <QVector>

class PlistValue
{
public:
    enum class Type {
        Null,
        String,
        Integer,
        Real,
        Boolean,
        Date,
        Data,
        Array,
        Dictionary
    };

    PlistValue();
    PlistValue(const PlistValue &other);
    PlistValue(PlistValue &&other) noexcept;
    PlistValue &operator=(const PlistValue &other);
    PlistValue &operator=(PlistValue &&other) noexcept;
    ~PlistValue();

    static PlistValue string(QString value);
    static PlistValue integer(qint64 value);
    static PlistValue real(double value);
    static PlistValue boolean(bool value);
    static PlistValue date(QDateTime value);
    static PlistValue data(QByteArray value);
    static PlistValue array(QVector<PlistValue> items);
    static PlistValue dictionary(QVector<QPair<QString, PlistValue>> items);

    Type type() const;
    bool isNull() const;
    bool isString() const;
    bool isInteger() const;
    bool isReal() const;
    bool isBoolean() const;
    bool isDate() const;
    bool isData() const;
    bool isArray() const;
    bool isDictionary() const;

    QString toString() const;
    qint64 toInteger() const;
    double toReal() const;
    bool toBoolean() const;
    QDateTime toDate() const;
    QByteArray toData() const;

    const QVector<PlistValue> &arrayItems() const;
    const QVector<QPair<QString, PlistValue>> &dictItems() const;

    const PlistValue *child(const QString &key) const;
    QString childString(const QString &key) const;
    bool contains(const QString &key) const;

    bool matches(const QString &query) const;

private:
    class Data;
    QSharedDataPointer<Data> d;
};
