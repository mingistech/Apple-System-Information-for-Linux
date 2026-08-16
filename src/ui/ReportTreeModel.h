#pragma once

#include <QStandardItemModel>

class ReportCategory;
class SystemReport;

class ReportTreeModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit ReportTreeModel(QObject *parent = nullptr);

    void setReport(const SystemReport *report);
    void clearReport();

    ReportCategory *categoryFromIndex(const QModelIndex &index) const;
    QModelIndex indexForCategory(const ReportCategory *category) const;

private:
    void appendCategory(QStandardItem *parentItem, ReportCategory *category);
};
