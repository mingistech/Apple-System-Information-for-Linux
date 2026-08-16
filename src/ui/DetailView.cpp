#include "ui/DetailView.h"

#include "model/LabelHumanizer.h"
#include "model/SystemReport.h"

#include <QHeaderView>
#include <QLabel>
#include <QSplitter>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <algorithm>

namespace {

bool isStructuralKey(const QString &key)
{
    return key == QLatin1String("_name") || key == QLatin1String("_items");
}

void configureTree(QTreeWidget *tree)
{
    tree->setRootIsDecorated(true);
    tree->setAlternatingRowColors(true);
    tree->setUniformRowHeights(true);
    tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tree->setWordWrap(false);
    tree->header()->setStretchLastSection(true);
}

} // namespace

DetailView::DetailView(QWidget *parent)
    : QWidget(parent)
{
    m_title = new QLabel(this);
    QFont titleFont = m_title->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() + 3);
    titleFont.setBold(true);
    m_title->setFont(titleFont);

    m_subtitle = new QLabel(this);
    QPalette muted = m_subtitle->palette();
    muted.setColor(QPalette::WindowText, muted.color(QPalette::PlaceholderText));
    m_subtitle->setPalette(muted);

    m_outline = new QTreeWidget(this);
    m_outline->setColumnCount(2);
    m_outline->setHeaderLabels({tr("Property"), tr("Value")});
    configureTree(m_outline);
    m_outline->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    m_table = new QTreeWidget(this);
    configureTree(m_table);
    m_table->setRootIsDecorated(false);
    m_table->setSortingEnabled(true);

    auto *contentSplitter = new QSplitter(Qt::Vertical, this);
    contentSplitter->addWidget(m_table);
    contentSplitter->addWidget(m_outline);
    contentSplitter->setStretchFactor(0, 2);
    contentSplitter->setStretchFactor(1, 3);
    contentSplitter->setChildrenCollapsible(false);

    m_emptyPage = new QWidget(this);
    auto *emptyLayout = new QVBoxLayout(m_emptyPage);
    auto *emptyLabel = new QLabel(tr("No information is available for this category."), m_emptyPage);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setWordWrap(true);
    emptyLayout->addStretch();
    emptyLayout->addWidget(emptyLabel);
    emptyLayout->addStretch();

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_emptyPage);
    m_stack->addWidget(contentSplitter);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(6);
    layout->addWidget(m_title);
    layout->addWidget(m_subtitle);
    layout->addWidget(m_stack, 1);

    m_title->setText(tr("System Report"));
    m_subtitle->setText(tr("Select a category in the sidebar to view its details."));
    m_stack->setCurrentWidget(m_emptyPage);
    m_table->hide();

    connect(m_table, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *current, QTreeWidgetItem *) {
        if (!m_tableMode || !m_category || !current) {
            return;
        }
        const int index = current->data(0, Qt::UserRole).toInt();
        const auto &items = m_category->items.arrayItems();
        if (index < 0 || index >= items.size()) {
            return;
        }
        m_outline->clear();
        showOutlineForItem(items.at(index), true);
        applySearch(m_query);
    });
}

void DetailView::showCategory(const ReportCategory *category)
{
    m_outline->clear();
    m_table->clear();
    m_table->setSortingEnabled(false);
    m_category = category;
    m_tableMode = false;

    if (!category) {
        clear();
        return;
    }

    m_title->setText(category->displayName);

    const int count = category->itemCount();
    if (count == 0) {
        m_subtitle->setText(tr("No information found in this report."));
        m_stack->setCurrentWidget(m_emptyPage);
        m_table->hide();
        return;
    }

    if (count == 1) {
        m_subtitle->clear();
    } else {
        m_subtitle->setText(tr("%n item(s)", nullptr, count));
    }

    const bool tableMode = shouldUseTable(category);
    m_tableMode = tableMode;
    m_table->setVisible(tableMode);
    if (tableMode) {
        populateTable(category);
    } else {
        populateOutline(category);
    }
    applySearch(m_query);
    m_stack->setCurrentIndex(1);
}

void DetailView::clear()
{
    m_title->setText(tr("System Report"));
    m_subtitle->setText(tr("Select a category in the sidebar to view its details."));
    m_outline->clear();
    m_table->clear();
    m_table->hide();
    m_category = nullptr;
    m_tableMode = false;
    m_stack->setCurrentWidget(m_emptyPage);
}

void DetailView::applySearch(const QString &query)
{
    m_query = query.trimmed();
    filterTree(m_outline, m_query);
    if (m_table->isVisible()) {
        filterTree(m_table, m_query);
    }
}

void DetailView::populateOutline(const ReportCategory *category)
{
    const QVector<PlistValue> &items = category->items.arrayItems();
    const bool wrapSingle = items.size() == 1 && !hasNestedItems(items.first())
        && items.first().isDictionary();

    if (wrapSingle) {
        showOutlineForItem(items.first(), true);
    } else {
        for (const PlistValue &item : items) {
            showOutlineForItem(item, false);
        }
        m_outline->header()->resizeSection(0, 240);
        expandUsefulNodes(m_outline);
    }
}

void DetailView::showOutlineForItem(const PlistValue &item, bool wrapSingle)
{
    if (wrapSingle && item.isDictionary() && !hasNestedItems(item)) {
        addDictionaryNodes(m_outline->invisibleRootItem(), item, m_category, true);
        m_outline->header()->resizeSection(0, 240);
        return;
    }

    if (item.isDictionary()) {
        auto *node = new QTreeWidgetItem(m_outline);
        node->setText(0, LabelHumanizer::itemTitle(item));
        addDictionaryNodes(node, item, m_category, true);
        node->setExpanded(true);
    } else {
        addValueNodes(m_outline->invisibleRootItem(), item, m_category);
    }
    m_outline->header()->resizeSection(0, 240);
    expandUsefulNodes(m_outline);
}

void DetailView::populateTable(const ReportCategory *category)
{
    const QVector<QString> columns = tableColumns(category);
    QStringList headers;
    headers.reserve(columns.size());
    for (const QString &key : columns) {
        headers << LabelHumanizer::keyName(key);
    }
    m_table->setColumnCount(columns.size());
    m_table->setHeaderLabels(headers);

    const QVector<PlistValue> &items = category->items.arrayItems();
    for (int i = 0; i < items.size(); ++i) {
        const PlistValue &item = items.at(i);
        auto *row = new QTreeWidgetItem(m_table);
        row->setData(0, Qt::UserRole, i);
        for (int column = 0; column < columns.size(); ++column) {
            const QString &key = columns.at(column);
            if (key == QLatin1String("_name")) {
                row->setText(column, LabelHumanizer::itemTitle(item));
                continue;
            }
            const PlistValue *value = item.child(key);
            if (!value) {
                continue;
            }
            row->setText(column, LabelHumanizer::displayValue(key, *value, propertyInfo(category, key)));
        }
    }

    m_table->setSortingEnabled(true);
    if (m_table->columnCount() > 0) {
        m_table->sortByColumn(0, Qt::AscendingOrder);
        m_table->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    }
    if (m_table->topLevelItemCount() > 0) {
        m_table->setCurrentItem(m_table->topLevelItem(0));
    }
}

void DetailView::addValueNodes(QTreeWidgetItem *parent, const PlistValue &value, const ReportCategory *category)
{
    if (value.isDictionary()) {
        addDictionaryNodes(parent, value, category, true);
        return;
    }
    if (value.isArray()) {
        if (value.arrayItems().isEmpty()) {
            auto *item = new QTreeWidgetItem(parent);
            item->setText(1, QStringLiteral("—"));
            return;
        }
        for (const PlistValue &child : value.arrayItems()) {
            if (child.isDictionary()) {
                auto *node = new QTreeWidgetItem(parent);
                node->setText(0, LabelHumanizer::itemTitle(child));
                addDictionaryNodes(node, child, category, true);
            } else {
                auto *node = new QTreeWidgetItem(parent);
                node->setText(0, LabelHumanizer::displayValue(QString(), child));
            }
        }
        return;
    }

    auto *item = new QTreeWidgetItem(parent);
    item->setText(1, LabelHumanizer::displayValue(QString(), value));
}

void DetailView::addDictionaryNodes(QTreeWidgetItem *parent, const PlistValue &dict,
                                   const ReportCategory *category, bool skipNameAndItems)
{
    QVector<QPair<QString, PlistValue>> entries = dict.dictItems();
    std::sort(entries.begin(), entries.end(), [&](const auto &a, const auto &b) {
        const PropertyInfo *infoA = propertyInfo(category, a.first);
        const PropertyInfo *infoB = propertyInfo(category, b.first);
        const int orderA = infoA ? infoA->order : 10000;
        const int orderB = infoB ? infoB->order : 10000;
        if (orderA != orderB) {
            return orderA < orderB;
        }
        return a.first < b.first;
    });

    const PlistValue *nestedItems = dict.child(QStringLiteral("_items"));

    for (const auto &entry : entries) {
        const QString &key = entry.first;
        const PlistValue &value = entry.second;
        if (skipNameAndItems && isStructuralKey(key)) {
            continue;
        }

        auto *item = new QTreeWidgetItem(parent);
        item->setText(0, LabelHumanizer::keyName(key));

        if (value.isDictionary() || value.isArray()) {
            if ((value.isArray() && value.arrayItems().isEmpty())
                || (value.isDictionary() && value.dictItems().isEmpty())) {
                item->setText(1, QStringLiteral("—"));
            } else {
                addValueNodes(item, value, category);
            }
        } else {
            item->setText(1, LabelHumanizer::displayValue(key, value, propertyInfo(category, key)));
        }
    }

    if (nestedItems && nestedItems->isArray()) {
        for (const PlistValue &child : nestedItems->arrayItems()) {
            auto *node = new QTreeWidgetItem(parent);
            node->setText(0, LabelHumanizer::itemTitle(child));
            if (child.isDictionary()) {
                addDictionaryNodes(node, child, category, true);
            } else {
                node->setText(1, LabelHumanizer::displayValue(QString(), child));
            }
        }
    }
}

QVector<QString> DetailView::tableColumns(const ReportCategory *category) const
{
    QVector<QString> columns;
    bool hasName = false;
    for (const PropertyInfo &info : category->properties) {
        if (info.key == QLatin1String("_name")) {
            hasName = true;
            columns.prepend(info.key);
            continue;
        }
        if (info.isColumn && !info.deprecated) {
            columns.append(info.key);
        }
    }
    if (!hasName) {
        columns.prepend(QStringLiteral("_name"));
    }
    return columns;
}

bool DetailView::shouldUseTable(const ReportCategory *category) const
{
    if (category->itemCount() < 2) {
        return false;
    }

    int columnCount = 0;
    for (const PropertyInfo &info : category->properties) {
        if (info.isColumn && info.key != QLatin1String("_name") && !info.deprecated) {
            ++columnCount;
        }
    }
    if (columnCount < 1) {
        return false;
    }

    int nested = 0;
    const int sample = qMin(category->itemCount(), 12);
    for (int i = 0; i < sample; ++i) {
        if (hasNestedItems(category->items.arrayItems().at(i))) {
            ++nested;
        }
    }
    return nested * 2 <= sample;
}

bool DetailView::hasNestedItems(const PlistValue &value) const
{
    if (!value.isDictionary()) {
        return false;
    }
    const PlistValue *items = value.child(QStringLiteral("_items"));
    return items && items->isArray() && !items->arrayItems().isEmpty();
}

const PropertyInfo *DetailView::propertyInfo(const ReportCategory *category, const QString &key) const
{
    if (!category) {
        return nullptr;
    }
    auto it = category->propertyByKey.constFind(key);
    if (it == category->propertyByKey.cend()) {
        return nullptr;
    }
    return &it.value();
}

void DetailView::expandUsefulNodes(QTreeWidget *tree)
{
    const int top = tree->topLevelItemCount();
    if (top == 0) {
        return;
    }
    if (top <= 12) {
        tree->expandToDepth(1);
        return;
    }
    for (int i = 0; i < qMin(top, 6); ++i) {
        tree->topLevelItem(i)->setExpanded(true);
    }
}

void DetailView::filterTree(QTreeWidget *tree, const QString &query)
{
    const int count = tree->topLevelItemCount();
    if (query.isEmpty()) {
        for (int i = 0; i < count; ++i) {
            filterItem(tree->topLevelItem(i), QString());
        }
        return;
    }
    for (int i = 0; i < count; ++i) {
        filterItem(tree->topLevelItem(i), query);
    }
}

bool DetailView::filterItem(QTreeWidgetItem *item, const QString &query)
{
    bool childMatch = false;
    for (int i = 0; i < item->childCount(); ++i) {
        childMatch = filterItem(item->child(i), query) || childMatch;
    }

    bool selfMatch = query.isEmpty();
    if (!query.isEmpty()) {
        for (int column = 0; column < item->columnCount(); ++column) {
            if (item->text(column).contains(query, Qt::CaseInsensitive)) {
                selfMatch = true;
                break;
            }
        }
    }

    const bool visible = selfMatch || childMatch || query.isEmpty();
    item->setHidden(!visible);
    if (childMatch && !query.isEmpty()) {
        item->setExpanded(true);
    }
    return visible;
}
