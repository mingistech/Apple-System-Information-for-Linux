#include "ui/ReportTreeModel.h"

#include "model/SystemReport.h"

#include <QStandardItem>
#include <functional>

namespace {
constexpr int kCategoryRole = Qt::UserRole + 1;
}

ReportTreeModel::ReportTreeModel(QObject *parent)
    : QStandardItemModel(parent)
{
    setColumnCount(1);
    setHorizontalHeaderLabels({QStringLiteral("Report")});
}

void ReportTreeModel::setReport(const SystemReport *report)
{
    QStandardItemModel::clear();
    setColumnCount(1);
    setHorizontalHeaderLabels({QStringLiteral("Report")});

    if (report) {
        for (ReportCategory *category : report->rootCategories()) {
            appendCategory(invisibleRootItem(), category);
        }
    }
}

void ReportTreeModel::clearReport()
{
    setReport(nullptr);
}

ReportCategory *ReportTreeModel::categoryFromIndex(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return nullptr;
    }
    QStandardItem *item = itemFromIndex(index);
    if (!item) {
        return nullptr;
    }
    return static_cast<ReportCategory *>(item->data(kCategoryRole).value<void *>());
}

QModelIndex ReportTreeModel::indexForCategory(const ReportCategory *category) const
{
    const std::function<QModelIndex(QStandardItem *)> find = [&](QStandardItem *parent) -> QModelIndex {
        const int rows = parent->rowCount();
        for (int row = 0; row < rows; ++row) {
            QStandardItem *item = parent->child(row);
            if (item->data(kCategoryRole).value<void *>() == category) {
                return item->index();
            }
            const QModelIndex nested = find(item);
            if (nested.isValid()) {
                return nested;
            }
        }
        return {};
    };
    return find(invisibleRootItem());
}

void ReportTreeModel::appendCategory(QStandardItem *parentItem, ReportCategory *category)
{
    auto *item = new QStandardItem(category->displayName);
    item->setEditable(false);
    item->setData(QVariant::fromValue(static_cast<void *>(category)), kCategoryRole);
    item->setToolTip(category->dataType);
    parentItem->appendRow(item);
    for (ReportCategory *child : category->children) {
        appendCategory(item, child);
    }
}
