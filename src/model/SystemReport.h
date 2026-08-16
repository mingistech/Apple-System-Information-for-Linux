#pragma once

#include "parser/PlistValue.h"

#include <memory>
#include <vector>
#include <QHash>
#include <QString>
#include <QVector>

struct PropertyInfo
{
    QString key;
    int order = 10000;
    bool isColumn = false;
    bool isOutlineColumn = false;
    bool isByteSize = false;
    bool deprecated = false;
    bool suppressLocalization = false;
};

struct ReportCategory
{
    QString dataType;
    QString parentDataType;
    QString displayName;
    PlistValue items;
    QVector<PropertyInfo> properties;
    QHash<QString, PropertyInfo> propertyByKey;
    QVector<ReportCategory *> children;
    ReportCategory *parent = nullptr;
    bool synthetic = false;
    int originalIndex = 0;

    bool matches(const QString &query) const;
    int itemCount() const;
};

class SystemReport
{
public:
    static std::unique_ptr<SystemReport> load(const QString &path, QString *error);

    QString filePath() const { return m_filePath; }
    QString fileName() const { return m_fileName; }

    const QVector<ReportCategory *> &rootCategories() const { return m_roots; }
    const std::vector<std::unique_ptr<ReportCategory>> &allCategories() const { return m_categories; }

private:
    SystemReport() = default;

    void buildCategories(const PlistValue &root);
    void attachParents();
    void insertOverviewNodes();
    void sortTree();
    PropertyInfo parsePropertyInfo(const QString &key, const PlistValue &meta) const;

    QString m_filePath;
    QString m_fileName;
    PlistValue m_root;
    std::vector<std::unique_ptr<ReportCategory>> m_categories;
    QVector<ReportCategory *> m_roots;
};
