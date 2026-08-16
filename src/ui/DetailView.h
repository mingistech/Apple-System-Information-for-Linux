#pragma once

#include "parser/PlistValue.h"

#include <QWidget>

class ReportCategory;
struct PropertyInfo;
class QLabel;
class QStackedWidget;
class QTreeWidget;
class QTreeWidgetItem;

class DetailView : public QWidget
{
    Q_OBJECT

public:
    explicit DetailView(QWidget *parent = nullptr);

    void showCategory(const ReportCategory *category);
    void clear();
    void applySearch(const QString &query);

private:
    void populateOutline(const ReportCategory *category);
    void populateTable(const ReportCategory *category);
    void showOutlineForItem(const PlistValue &item, bool wrapSingle);
    void addValueNodes(QTreeWidgetItem *parent, const PlistValue &value, const ReportCategory *category);
    void addDictionaryNodes(QTreeWidgetItem *parent, const PlistValue &dict, const ReportCategory *category, bool skipNameAndItems);
    QVector<QString> tableColumns(const ReportCategory *category) const;
    bool shouldUseTable(const ReportCategory *category) const;
    bool hasNestedItems(const PlistValue &value) const;
    const PropertyInfo *propertyInfo(const ReportCategory *category, const QString &key) const;
    void expandUsefulNodes(QTreeWidget *tree);
    void filterTree(QTreeWidget *tree, const QString &query);
    bool filterItem(QTreeWidgetItem *item, const QString &query);

    QLabel *m_title = nullptr;
    QLabel *m_subtitle = nullptr;
    QStackedWidget *m_stack = nullptr;
    QWidget *m_emptyPage = nullptr;
    QTreeWidget *m_outline = nullptr;
    QTreeWidget *m_table = nullptr;
    const ReportCategory *m_category = nullptr;
    QString m_query;
    bool m_tableMode = false;
};
