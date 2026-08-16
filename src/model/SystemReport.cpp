#include "model/SystemReport.h"

#include "model/LabelHumanizer.h"
#include "parser/SpxParser.h"

#include <QFileInfo>
#include <algorithm>

namespace {

const QStringList kRootOrder = {
    QStringLiteral("SPHardwareDataType"),
    QStringLiteral("SPNetworkDataType"),
    QStringLiteral("SPSoftwareDataType"),
};

const QStringList kChildOrder = {
    QStringLiteral("SPHardwareDataType.overview"),
    QStringLiteral("SPSoftwareDataType.overview"),
    QStringLiteral("SPNetworkDataType.overview"),
    QStringLiteral("SPParallelATADataType"),
    QStringLiteral("SPAudioDataType"),
    QStringLiteral("SPBluetoothDataType"),
    QStringLiteral("SPCameraDataType"),
    QStringLiteral("SPCardReaderDataType"),
    QStringLiteral("SPDiagnosticsDataType"),
    QStringLiteral("SPDiscBurningDataType"),
    QStringLiteral("SPEthernetDataType"),
    QStringLiteral("SPFibreChannelDataType"),
    QStringLiteral("SPFireWireDataType"),
    QStringLiteral("SPDisplaysDataType"),
    QStringLiteral("SPHardwareRAIDDataType"),
    QStringLiteral("SPiBridgeDataType"),
    QStringLiteral("SPMemoryDataType"),
    QStringLiteral("SPNVMeDataType"),
    QStringLiteral("SPPCIDataType"),
    QStringLiteral("SPParallelSCSIDataType"),
    QStringLiteral("SPPowerDataType"),
    QStringLiteral("SPPrintersDataType"),
    QStringLiteral("SPSASDataType"),
    QStringLiteral("SPSerialATADataType"),
    QStringLiteral("SPSPIDataType"),
    QStringLiteral("SPSecureElementDataType"),
    QStringLiteral("SPSmartCardsDataType"),
    QStringLiteral("SPStorageDataType"),
    QStringLiteral("SPThunderboltDataType"),
    QStringLiteral("SPUSBHostDataType"),
    QStringLiteral("SPUSBDataType"),
    QStringLiteral("SPAirPortDataType"),
    QStringLiteral("SPFirewallDataType"),
    QStringLiteral("SPNetworkLocationDataType"),
    QStringLiteral("SPNetworkVolumeDataType"),
    QStringLiteral("SPWWANDataType"),
    QStringLiteral("SPModemDataType"),
    QStringLiteral("SPUniversalAccessDataType"),
    QStringLiteral("SPApplicationsDataType"),
    QStringLiteral("SPDeveloperToolsDataType"),
    QStringLiteral("SPDisabledSoftwareDataType"),
    QStringLiteral("SPExtensionsDataType"),
    QStringLiteral("SPFontsDataType"),
    QStringLiteral("SPFrameworksDataType"),
    QStringLiteral("SPInstallHistoryDataType"),
    QStringLiteral("SPInternationalDataType"),
    QStringLiteral("SPLegacySoftwareDataType"),
    QStringLiteral("SPLogsDataType"),
    QStringLiteral("SPManagedClientDataType"),
    QStringLiteral("SPPrefPaneDataType"),
    QStringLiteral("SPPrintersSoftwareDataType"),
    QStringLiteral("SPConfigurationProfileDataType"),
    QStringLiteral("SPRawCameraDataType"),
    QStringLiteral("SPStartupItemDataType"),
    QStringLiteral("SPSyncServicesDataType"),
    QStringLiteral("SPAppleVirtualPlatformDataType"),
};

int orderIndex(const QStringList &list, const QString &key, int fallback)
{
    const int index = list.indexOf(key);
    return index >= 0 ? index : fallback;
}

bool truthy(const PlistValue &value)
{
    if (value.isBoolean()) {
        return value.toBoolean();
    }
    if (value.isString()) {
        const QString text = value.toString();
        return text.compare(QLatin1String("YES"), Qt::CaseInsensitive) == 0
            || text.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0;
    }
    if (value.isInteger()) {
        return value.toInteger() != 0;
    }
    return false;
}

} // namespace

bool ReportCategory::matches(const QString &query) const
{
    if (query.isEmpty()) {
        return true;
    }
    if (displayName.contains(query, Qt::CaseInsensitive)) {
        return true;
    }
    if (dataType.contains(query, Qt::CaseInsensitive)) {
        return true;
    }
    return items.matches(query);
}

int ReportCategory::itemCount() const
{
    return items.isArray() ? items.arrayItems().size() : 0;
}

std::unique_ptr<SystemReport> SystemReport::load(const QString &path, QString *error)
{
    const SpxParser::Result parsed = SpxParser::parseFile(path);
    if (!parsed.ok) {
        if (error) {
            if (parsed.line > 0) {
                *error = QStringLiteral("%1 (line %2, column %3)")
                             .arg(parsed.error)
                             .arg(parsed.line)
                             .arg(parsed.column);
            } else {
                *error = parsed.error;
            }
        }
        return nullptr;
    }

    if (!SpxParser::looksLikeSystemReport(parsed.root)) {
        if (error) {
            *error = QStringLiteral("The selected file does not appear to be a valid Apple System Information SPX report.");
        }
        return nullptr;
    }

    auto report = std::unique_ptr<SystemReport>(new SystemReport);
    report->m_filePath = path;
    report->m_fileName = QFileInfo(path).fileName();
    report->m_root = parsed.root;
    report->buildCategories(parsed.root);
    report->attachParents();
    report->insertOverviewNodes();
    report->sortTree();
    return report;
}

void SystemReport::buildCategories(const PlistValue &root)
{
    int index = 0;
    for (const PlistValue &entry : root.arrayItems()) {
        if (!entry.isDictionary()) {
            continue;
        }
        const QString dataType = entry.childString(QStringLiteral("_dataType"));
        if (dataType.isEmpty()) {
            continue;
        }

        auto category = std::make_unique<ReportCategory>();
        category->dataType = dataType;
        category->parentDataType = entry.childString(QStringLiteral("_parentDataType"));
        category->displayName = LabelHumanizer::dataTypeName(dataType);
        category->originalIndex = index++;

        if (const PlistValue *items = entry.child(QStringLiteral("_items"))) {
            category->items = *items;
        } else {
            category->items = PlistValue::array({});
        }

        if (const PlistValue *properties = entry.child(QStringLiteral("_properties"))) {
            for (const auto &prop : properties->dictItems()) {
                PropertyInfo info = parsePropertyInfo(prop.first, prop.second);
                category->propertyByKey.insert(info.key, info);
                category->properties.append(info);
            }
            std::sort(category->properties.begin(), category->properties.end(),
                      [](const PropertyInfo &a, const PropertyInfo &b) {
                          if (a.order != b.order) {
                              return a.order < b.order;
                          }
                          return a.key < b.key;
                      });
        }

        m_categories.push_back(std::move(category));
    }
}

void SystemReport::attachParents()
{
    QHash<QString, ReportCategory *> byType;
    byType.reserve(m_categories.size());
    for (const auto &category : m_categories) {
        byType.insert(category->dataType, category.get());
    }

    for (const auto &category : m_categories) {
        ReportCategory *node = category.get();
        if (node->parentDataType.isEmpty()
            || node->parentDataType == QLatin1String("SPRootDataType")
            || node->parentDataType == node->dataType) {
            m_roots.append(node);
            continue;
        }

        ReportCategory *parent = byType.value(node->parentDataType, nullptr);
        if (!parent || parent == node) {
            m_roots.append(node);
            continue;
        }

        node->parent = parent;
        parent->children.append(node);
    }
}

void SystemReport::insertOverviewNodes()
{
    std::vector<std::unique_ptr<ReportCategory>> extras;
    for (const auto &category : m_categories) {
        ReportCategory *node = category.get();
        if (node->children.isEmpty() || node->itemCount() == 0) {
            continue;
        }

        auto overview = std::make_unique<ReportCategory>();
        overview->dataType = node->dataType + QStringLiteral(".overview");
        overview->parentDataType = node->dataType;
        overview->items = node->items;
        overview->properties = node->properties;
        overview->propertyByKey = node->propertyByKey;
        overview->parent = node;
        overview->synthetic = true;
        overview->originalIndex = -1;

        QString overviewName = QStringLiteral("%1 Overview").arg(node->displayName);
        if (node->itemCount() == 1) {
            const QString itemName = node->items.arrayItems().first().childString(QStringLiteral("_name"));
            if (!itemName.isEmpty()) {
                overviewName = LabelHumanizer::keyName(itemName);
            }
        }
        if (node->dataType == QLatin1String("SPSoftwareDataType")) {
            overviewName = QStringLiteral("System Software Overview");
        }
        overview->displayName = overviewName;

        node->children.prepend(overview.get());
        extras.push_back(std::move(overview));
    }
    for (auto &extra : extras) {
        m_categories.push_back(std::move(extra));
    }
}

void SystemReport::sortTree()
{
    auto compareNodes = [](const ReportCategory *a, const ReportCategory *b) {
        const int aRoot = orderIndex(kRootOrder, a->dataType, 100);
        const int bRoot = orderIndex(kRootOrder, b->dataType, 100);
        if (aRoot != bRoot) {
            return aRoot < bRoot;
        }
        const int aChild = orderIndex(kChildOrder, a->dataType, 500 + a->originalIndex);
        const int bChild = orderIndex(kChildOrder, b->dataType, 500 + b->originalIndex);
        if (aChild != bChild) {
            return aChild < bChild;
        }
        return a->originalIndex < b->originalIndex;
    };

    std::sort(m_roots.begin(), m_roots.end(), compareNodes);
    for (const auto &category : m_categories) {
        std::sort(category->children.begin(), category->children.end(), compareNodes);
    }
}

PropertyInfo SystemReport::parsePropertyInfo(const QString &key, const PlistValue &meta) const
{
    PropertyInfo info;
    info.key = key;
    if (!meta.isDictionary()) {
        return info;
    }

    if (const PlistValue *order = meta.child(QStringLiteral("_order"))) {
        if (order->isInteger()) {
            info.order = static_cast<int>(order->toInteger());
        } else if (order->isString()) {
            info.order = order->toString().toInt();
        }
    }
    if (const PlistValue *column = meta.child(QStringLiteral("_isColumn"))) {
        info.isColumn = truthy(*column);
    }
    if (const PlistValue *outline = meta.child(QStringLiteral("_isOutlineColumn"))) {
        info.isOutlineColumn = truthy(*outline);
    }
    if (const PlistValue *byteSize = meta.child(QStringLiteral("_isByteSize"))) {
        info.isByteSize = truthy(*byteSize);
    }
    if (const PlistValue *deprecated = meta.child(QStringLiteral("_deprecated"))) {
        info.deprecated = truthy(*deprecated);
    }
    if (const PlistValue *suppress = meta.child(QStringLiteral("_suppressLocalization"))) {
        info.suppressLocalization = truthy(*suppress);
    }
    return info;
}
