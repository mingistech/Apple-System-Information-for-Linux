#include "model/LabelHumanizer.h"

#include "model/SystemReport.h"
#include "parser/PlistValue.h"

#include <QHash>
#include <QLocale>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>

namespace {

const QHash<QString, QString> &dataTypeNames()
{
    static const QHash<QString, QString> names = {
        {QStringLiteral("SPHardwareDataType"), QStringLiteral("Hardware")},
        {QStringLiteral("SPNetworkDataType"), QStringLiteral("Network")},
        {QStringLiteral("SPSoftwareDataType"), QStringLiteral("Software")},
        {QStringLiteral("SPMemoryDataType"), QStringLiteral("Memory")},
        {QStringLiteral("SPDisplaysDataType"), QStringLiteral("Graphics/Displays")},
        {QStringLiteral("SPStorageDataType"), QStringLiteral("Storage")},
        {QStringLiteral("SPUSBDataType"), QStringLiteral("USB")},
        {QStringLiteral("SPUSBHostDataType"), QStringLiteral("USB")},
        {QStringLiteral("SPThunderboltDataType"), QStringLiteral("Thunderbolt/USB4")},
        {QStringLiteral("SPBluetoothDataType"), QStringLiteral("Bluetooth")},
        {QStringLiteral("SPAudioDataType"), QStringLiteral("Audio")},
        {QStringLiteral("SPAirPortDataType"), QStringLiteral("Wi-Fi")},
        {QStringLiteral("SPEthernetDataType"), QStringLiteral("Ethernet")},
        {QStringLiteral("SPFirewallDataType"), QStringLiteral("Firewall")},
        {QStringLiteral("SPApplicationsDataType"), QStringLiteral("Applications")},
        {QStringLiteral("SPExtensionsDataType"), QStringLiteral("Extensions")},
        {QStringLiteral("SPInstallHistoryDataType"), QStringLiteral("Installations")},
        {QStringLiteral("SPPowerDataType"), QStringLiteral("Power")},
        {QStringLiteral("SPCameraDataType"), QStringLiteral("Camera")},
        {QStringLiteral("SPPrintersDataType"), QStringLiteral("Printers")},
        {QStringLiteral("SPNVMeDataType"), QStringLiteral("NVMExpress")},
        {QStringLiteral("SPSerialATADataType"), QStringLiteral("SATA/SATA Express")},
        {QStringLiteral("SPNetworkLocationDataType"), QStringLiteral("Locations")},
        {QStringLiteral("SPConfigurationProfileDataType"), QStringLiteral("Profiles")},
        {QStringLiteral("SPDisabledSoftwareDataType"), QStringLiteral("Disabled Software")},
        {QStringLiteral("SPLegacySoftwareDataType"), QStringLiteral("Legacy Software")},
        {QStringLiteral("SPPrefPaneDataType"), QStringLiteral("Preference Panes")},
        {QStringLiteral("SPDeveloperToolsDataType"), QStringLiteral("Developer")},
        {QStringLiteral("SPUniversalAccessDataType"), QStringLiteral("Accessibility")},
        {QStringLiteral("SPSecureElementDataType"), QStringLiteral("Secure Element")},
        {QStringLiteral("SPiBridgeDataType"), QStringLiteral("iBridge")},
        {QStringLiteral("SPCardReaderDataType"), QStringLiteral("Card Reader")},
        {QStringLiteral("SPDiagnosticsDataType"), QStringLiteral("Diagnostics")},
        {QStringLiteral("SPDiscBurningDataType"), QStringLiteral("Disc Burning")},
        {QStringLiteral("SPFibreChannelDataType"), QStringLiteral("Fibre Channel")},
        {QStringLiteral("SPFireWireDataType"), QStringLiteral("FireWire")},
        {QStringLiteral("SPHardwareRAIDDataType"), QStringLiteral("Hardware RAID")},
        {QStringLiteral("SPPCIDataType"), QStringLiteral("PCI")},
        {QStringLiteral("SPParallelSCSIDataType"), QStringLiteral("Parallel SCSI")},
        {QStringLiteral("SPParallelATADataType"), QStringLiteral("ATA")},
        {QStringLiteral("SPSASDataType"), QStringLiteral("SAS")},
        {QStringLiteral("SPSPIDataType"), QStringLiteral("SPI")},
        {QStringLiteral("SPSmartCardsDataType"), QStringLiteral("SmartCards")},
        {QStringLiteral("SPStartupItemDataType"), QStringLiteral("Startup Items")},
        {QStringLiteral("SPSyncServicesDataType"), QStringLiteral("Sync Services")},
        {QStringLiteral("SPNetworkVolumeDataType"), QStringLiteral("Volumes")},
        {QStringLiteral("SPWWANDataType"), QStringLiteral("WWAN")},
        {QStringLiteral("SPModemDataType"), QStringLiteral("Modem")},
        {QStringLiteral("SPPrintersSoftwareDataType"), QStringLiteral("Printer Software")},
        {QStringLiteral("SPFontsDataType"), QStringLiteral("Fonts")},
        {QStringLiteral("SPFrameworksDataType"), QStringLiteral("Frameworks")},
        {QStringLiteral("SPLogsDataType"), QStringLiteral("Logs")},
        {QStringLiteral("SPManagedClientDataType"), QStringLiteral("Managed Client")},
        {QStringLiteral("SPInternationalDataType"), QStringLiteral("International")},
        {QStringLiteral("SPRawCameraDataType"), QStringLiteral("Raw Camera")},
        {QStringLiteral("SPAppleVirtualPlatformDataType"), QStringLiteral("Virtual Platform")},
        {QStringLiteral("SPRootDataType"), QStringLiteral("System Report")},
    };
    return names;
}

const QHash<QString, QString> &keyNames()
{
    static const QHash<QString, QString> names = {
        {QStringLiteral("machine_name"), QStringLiteral("Model Name")},
        {QStringLiteral("machine_model"), QStringLiteral("Model Identifier")},
        {QStringLiteral("chip_type"), QStringLiteral("Chip")},
        {QStringLiteral("cpu_type"), QStringLiteral("Processor Name")},
        {QStringLiteral("current_processor_speed"), QStringLiteral("Processor Speed")},
        {QStringLiteral("number_processors"), QStringLiteral("Total Number of Cores")},
        {QStringLiteral("physical_memory"), QStringLiteral("Memory")},
        {QStringLiteral("serial_number"), QStringLiteral("Serial Number")},
        {QStringLiteral("platform_UUID"), QStringLiteral("Hardware UUID")},
        {QStringLiteral("boot_rom_version"), QStringLiteral("System Firmware Version")},
        {QStringLiteral("os_loader_version"), QStringLiteral("OS Loader Version")},
        {QStringLiteral("provisioning_UDID"), QStringLiteral("Provisioning UDID")},
        {QStringLiteral("activation_lock_status"), QStringLiteral("Activation Lock Status")},
        {QStringLiteral("model_number"), QStringLiteral("Model Number")},
        {QStringLiteral("hardware_overview"), QStringLiteral("Hardware Overview")},
        {QStringLiteral("os_overview"), QStringLiteral("System Software Overview")},
        {QStringLiteral("os_version"), QStringLiteral("System Version")},
        {QStringLiteral("kernel_version"), QStringLiteral("Kernel Version")},
        {QStringLiteral("boot_volume"), QStringLiteral("Boot Volume")},
        {QStringLiteral("boot_mode"), QStringLiteral("Boot Mode")},
        {QStringLiteral("computer_name"), QStringLiteral("Computer Name")},
        {QStringLiteral("user_name"), QStringLiteral("User Name")},
        {QStringLiteral("local_host_name"), QStringLiteral("Computer Name")},
        {QStringLiteral("secure_vm"), QStringLiteral("Secure Virtual Memory")},
        {QStringLiteral("system_integrity"), QStringLiteral("System Integrity Protection")},
        {QStringLiteral("uptime"), QStringLiteral("Time Since Boot")},
        {QStringLiteral("arch_kind"), QStringLiteral("Kind")},
        {QStringLiteral("obtained_from"), QStringLiteral("Obtained From")},
        {QStringLiteral("lastModified"), QStringLiteral("Last Modified")},
        {QStringLiteral("signed_by"), QStringLiteral("Signed by")},
        {QStringLiteral("path"), QStringLiteral("Location")},
        {QStringLiteral("version"), QStringLiteral("Version")},
        {QStringLiteral("info"), QStringLiteral("Get Info String")},
        {QStringLiteral("dimm_manufacturer"), QStringLiteral("Manufacturer")},
        {QStringLiteral("dimm_type"), QStringLiteral("Type")},
        {QStringLiteral("dimm_size"), QStringLiteral("Size")},
        {QStringLiteral("dimm_speed"), QStringLiteral("Speed")},
        {QStringLiteral("dimm_status"), QStringLiteral("Status")},
        {QStringLiteral("dimm_serial_number"), QStringLiteral("Serial Number")},
        {QStringLiteral("SPMemoryDataType"), QStringLiteral("Memory")},
        {QStringLiteral("size_in_bytes"), QStringLiteral("Capacity")},
        {QStringLiteral("free_space_in_bytes"), QStringLiteral("Free")},
        {QStringLiteral("size"), QStringLiteral("Size")},
        {QStringLiteral("free_space"), QStringLiteral("Free")},
        {QStringLiteral("bsd_name"), QStringLiteral("BSD Name")},
        {QStringLiteral("file_system"), QStringLiteral("File System")},
        {QStringLiteral("mount_point"), QStringLiteral("Mount Point")},
        {QStringLiteral("writable"), QStringLiteral("Writable")},
        {QStringLiteral("volume_uuid"), QStringLiteral("Volume UUID")},
        {QStringLiteral("physical_drive"), QStringLiteral("Physical Drive")},
        {QStringLiteral("device_name"), QStringLiteral("Device Name")},
        {QStringLiteral("is_internal_disk"), QStringLiteral("Internal")},
        {QStringLiteral("medium_type"), QStringLiteral("Medium Type")},
        {QStringLiteral("protocol"), QStringLiteral("Protocol")},
        {QStringLiteral("smart_status"), QStringLiteral("S.M.A.R.T. Status")},
        {QStringLiteral("ignore_ownership"), QStringLiteral("Ignore Ownership")},
        {QStringLiteral("USBDeviceKeyLinkSpeed"), QStringLiteral("Speed")},
        {QStringLiteral("USBDeviceKeyProductID"), QStringLiteral("Product ID")},
        {QStringLiteral("USBDeviceKeyVendorID"), QStringLiteral("Vendor ID")},
        {QStringLiteral("USBDeviceKeyVendorName"), QStringLiteral("Vendor")},
        {QStringLiteral("USBDeviceKeySerialNumber"), QStringLiteral("Serial Number")},
        {QStringLiteral("USBDeviceKeyProductVersion"), QStringLiteral("Version")},
        {QStringLiteral("USBDeviceKeyUSB4Tunnel"), QStringLiteral("USB4 Tunnel")},
        {QStringLiteral("USBDeviceKeyPowerAllocation"), QStringLiteral("Current Available")},
        {QStringLiteral("USBKeyHardwareType"), QStringLiteral("Type")},
        {QStringLiteral("USBKeyLocationID"), QStringLiteral("Location ID")},
        {QStringLiteral("Driver"), QStringLiteral("Driver")},
        {QStringLiteral("spdisplays_vendor"), QStringLiteral("Vendor")},
        {QStringLiteral("sppci_model"), QStringLiteral("Chipset Model")},
        {QStringLiteral("sppci_cores"), QStringLiteral("Total Number of Cores")},
        {QStringLiteral("sppci_bus"), QStringLiteral("Bus")},
        {QStringLiteral("sppci_device_type"), QStringLiteral("Type")},
        {QStringLiteral("spdisplays_mtlgpufamilysupport"), QStringLiteral("Metal Support")},
        {QStringLiteral("spdisplays_ndrvs"), QStringLiteral("Displays")},
        {QStringLiteral("spdisplays_main"), QStringLiteral("Main Display")},
        {QStringLiteral("spdisplays_mirror"), QStringLiteral("Mirror")},
        {QStringLiteral("spdisplays_online"), QStringLiteral("Online")},
        {QStringLiteral("spdisplays_resolution"), QStringLiteral("Resolution")},
        {QStringLiteral("spdisplays_pixelresolution"), QStringLiteral("Pixel Resolution")},
        {QStringLiteral("spdisplays_rotation"), QStringLiteral("Rotation")},
        {QStringLiteral("_spdisplays_pixels"), QStringLiteral("Pixels")},
        {QStringLiteral("_spdisplays_resolution"), QStringLiteral("UI Looks like")},
        {QStringLiteral("spdisplays_display"), QStringLiteral("Display")},
        {QStringLiteral("coreaudio_device_manufacturer"), QStringLiteral("Manufacturer")},
        {QStringLiteral("coreaudio_device_srate"), QStringLiteral("Sample Rate")},
        {QStringLiteral("coreaudio_device_transport"), QStringLiteral("Transport")},
        {QStringLiteral("hardware"), QStringLiteral("Hardware")},
        {QStringLiteral("interface"), QStringLiteral("BSD Device Name")},
        {QStringLiteral("type"), QStringLiteral("Type")},
        {QStringLiteral("spnetwork_service_order"), QStringLiteral("Service Order")},
        {QStringLiteral("IPv4"), QStringLiteral("IPv4")},
        {QStringLiteral("IPv6"), QStringLiteral("IPv6")},
        {QStringLiteral("Ethernet"), QStringLiteral("Ethernet")},
        {QStringLiteral("Proxies"), QStringLiteral("Proxies")},
        {QStringLiteral("MAC Address"), QStringLiteral("MAC Address")},
        {QStringLiteral("ConfigMethod"), QStringLiteral("Configuration Method")},
        {QStringLiteral("MediaSubType"), QStringLiteral("Media")},
        {QStringLiteral("MediaOptions"), QStringLiteral("Media Options")},
        {QStringLiteral("ExceptionsList"), QStringLiteral("Bypass Proxy Settings for these Hosts and Domains")},
        {QStringLiteral("FTPPassive"), QStringLiteral("Use Passive FTP Mode")},
        {QStringLiteral("ua_info"), QStringLiteral("Universal Access Information")},
        {QStringLiteral("voiceover"), QStringLiteral("VoiceOver")},
        {QStringLiteral("cursor_mag"), QStringLiteral("Cursor Magnification")},
        {QStringLiteral("flash_screen"), QStringLiteral("Flash Screen")},
        {QStringLiteral("mouse_keys"), QStringLiteral("Mouse Keys")},
        {QStringLiteral("slow_keys"), QStringLiteral("Slow Keys")},
        {QStringLiteral("sticky_keys"), QStringLiteral("Sticky Keys")},
        {QStringLiteral("zoomMode"), QStringLiteral("Zoom Mode")},
        {QStringLiteral("keyboardZoom"), QStringLiteral("Keyboard Zoom")},
        {QStringLiteral("scrollZoom"), QStringLiteral("Scroll Zoom")},
        {QStringLiteral("display"), QStringLiteral("Display")},
        {QStringLiteral("contrast"), QStringLiteral("Contrast")},
    };
    return names;
}

const QHash<QString, QString> &valueNames()
{
    static const QHash<QString, QString> names = {
        {QStringLiteral("activation_lock_disabled"), QStringLiteral("Disabled")},
        {QStringLiteral("activation_lock_enabled"), QStringLiteral("Enabled")},
        {QStringLiteral("normal_boot"), QStringLiteral("Normal")},
        {QStringLiteral("secure_boot"), QStringLiteral("Secure")},
        {QStringLiteral("secure_vm_enabled"), QStringLiteral("Enabled")},
        {QStringLiteral("secure_vm_disabled"), QStringLiteral("Disabled")},
        {QStringLiteral("integrity_enabled"), QStringLiteral("Enabled")},
        {QStringLiteral("integrity_disabled"), QStringLiteral("Disabled")},
        {QStringLiteral("arch_i64"), QStringLiteral("Intel")},
        {QStringLiteral("arch_arm"), QStringLiteral("Apple Silicon")},
        {QStringLiteral("arch_arm_i64"), QStringLiteral("Universal")},
        {QStringLiteral("arch_i32"), QStringLiteral("Intel 32-bit")},
        {QStringLiteral("arch_other"), QStringLiteral("Other")},
        {QStringLiteral("identified_developer"), QStringLiteral("Identified Developer")},
        {QStringLiteral("mac_app_store"), QStringLiteral("Mac App Store")},
        {QStringLiteral("apple"), QStringLiteral("Apple")},
        {QStringLiteral("apple_signed"), QStringLiteral("Apple")},
        {QStringLiteral("unknown"), QStringLiteral("Unknown")},
        {QStringLiteral("spdisplays_yes"), QStringLiteral("Yes")},
        {QStringLiteral("spdisplays_no"), QStringLiteral("No")},
        {QStringLiteral("spdisplays_off"), QStringLiteral("Off")},
        {QStringLiteral("spdisplays_on"), QStringLiteral("On")},
        {QStringLiteral("spdisplays_supported"), QStringLiteral("Supported")},
        {QStringLiteral("spdisplays_metal4"), QStringLiteral("Metal 4")},
        {QStringLiteral("spdisplays_metal3"), QStringLiteral("Metal 3")},
        {QStringLiteral("spdisplays_metal2"), QStringLiteral("Metal 2")},
        {QStringLiteral("spdisplays_builtin"), QStringLiteral("Built-In")},
        {QStringLiteral("spdisplays_gpu"), QStringLiteral("GPU")},
        {QStringLiteral("spdisplays_1080p"), QStringLiteral("1080p")},
        {QStringLiteral("sppci_vendor_Apple"), QStringLiteral("Apple")},
        {QStringLiteral("coreaudio_device_type_usb"), QStringLiteral("USB")},
        {QStringLiteral("coreaudio_device_type_builtin"), QStringLiteral("Built-in")},
        {QStringLiteral("yes"), QStringLiteral("Yes")},
        {QStringLiteral("no"), QStringLiteral("No")},
        {QStringLiteral("on"), QStringLiteral("On")},
        {QStringLiteral("off"), QStringLiteral("Off")},
        {QStringLiteral("true"), QStringLiteral("Yes")},
        {QStringLiteral("false"), QStringLiteral("No")},
        {QStringLiteral("htt_enabled"), QStringLiteral("Enabled")},
        {QStringLiteral("htt_disabled"), QStringLiteral("Disabled")},
        {QStringLiteral("black_on_white"), QStringLiteral("Black on White")},
        {QStringLiteral("white_on_black"), QStringLiteral("White on Black")},
        {QStringLiteral("zoom_full_screen"), QStringLiteral("Full Screen")},
        {QStringLiteral("ssd"), QStringLiteral("Solid State")},
        {QStringLiteral("hdd"), QStringLiteral("Rotational")},
        {QStringLiteral("unknown_partition_map_type"), QStringLiteral("Unknown")},
        {QStringLiteral("none"), QStringLiteral("None")},
    };
    return names;
}

const QStringList kPrefixesToStrip = {
    QStringLiteral("USBDeviceKey"),
    QStringLiteral("USBKey"),
    QStringLiteral("_spdisplays_"),
    QStringLiteral("spdisplays_"),
    QStringLiteral("sppci_"),
    QStringLiteral("spsata_"),
    QStringLiteral("spata_"),
    QStringLiteral("spusb_"),
    QStringLiteral("spnetwork_"),
    QStringLiteral("coreaudio_device_"),
    QStringLiteral("coreaudio_"),
    QStringLiteral("dimm_"),
};

QString formatUptime(const QString &raw)
{
    static const QRegularExpression re(QStringLiteral("^up\\s+(\\d+):(\\d+):(\\d+):(\\d+)$"));
    const QRegularExpressionMatch match = re.match(raw.trimmed());
    if (!match.hasMatch()) {
        return QString();
    }
    const int days = match.captured(1).toInt();
    const int hours = match.captured(2).toInt();
    const int minutes = match.captured(3).toInt();
    QStringList parts;
    if (days > 0) {
        parts << QStringLiteral("%1 %2").arg(days).arg(days == 1 ? QStringLiteral("day") : QStringLiteral("days"));
    }
    if (hours > 0) {
        parts << QStringLiteral("%1 %2").arg(hours).arg(hours == 1 ? QStringLiteral("hour") : QStringLiteral("hours"));
    }
    if (minutes > 0 || parts.isEmpty()) {
        parts << QStringLiteral("%1 %2").arg(minutes).arg(minutes == 1 ? QStringLiteral("minute") : QStringLiteral("minutes"));
    }
    return parts.join(QStringLiteral(", "));
}

QString formatProcessorCores(const QString &raw)
{
    static const QRegularExpression re(QStringLiteral("^proc\\s+(\\d+):(\\d+):(\\d+):(\\d+)$"));
    const QRegularExpressionMatch match = re.match(raw.trimmed());
    if (!match.hasMatch()) {
        return QString();
    }
    const int total = match.captured(1).toInt();
    const int performance = match.captured(3).toInt();
    const int efficiency = match.captured(4).toInt();
    if (performance > 0 && efficiency > 0) {
        return QStringLiteral("%1 (%2 performance and %3 efficiency)")
            .arg(total)
            .arg(performance)
            .arg(efficiency);
    }
    return QString::number(total);
}

} // namespace

QString LabelHumanizer::dataTypeName(const QString &dataType)
{
    if (dataTypeNames().contains(dataType)) {
        return dataTypeNames().value(dataType);
    }

    QString name = dataType;
    if (name.startsWith(QLatin1String("SP"))) {
        name = name.mid(2);
    }
    if (name.endsWith(QLatin1String("DataType"))) {
        name.chop(8);
    }
    name = splitIdentifier(name);
    return name.isEmpty() ? dataType : name;
}

QString LabelHumanizer::keyName(const QString &key)
{
    if (keyNames().contains(key)) {
        return keyNames().value(key);
    }
    if (dataTypeNames().contains(key)) {
        return dataTypeNames().value(key);
    }

    const QString stripped = stripKnownPrefix(key);
    return titleCase(splitIdentifier(stripped));
}

QString LabelHumanizer::displayValue(const QString &key, const PlistValue &value, const PropertyInfo *info)
{
    if (value.isNull()) {
        return QString();
    }

    if (info && info->isByteSize && (value.isInteger() || value.isReal())) {
        const qint64 bytes = value.isInteger() ? value.toInteger() : static_cast<qint64>(value.toReal());
        return formatByteSize(bytes);
    }

    if ((key == QLatin1String("size_in_bytes") || key == QLatin1String("free_space_in_bytes"))
        && (value.isInteger() || value.isReal())) {
        const qint64 bytes = value.isInteger() ? value.toInteger() : static_cast<qint64>(value.toReal());
        return formatByteSize(bytes);
    }

    if (value.isBoolean()) {
        return value.toBoolean() ? QStringLiteral("Yes") : QStringLiteral("No");
    }
    if (value.isInteger()) {
        return QLocale::system().toString(value.toInteger());
    }
    if (value.isReal()) {
        return QLocale::system().toString(value.toReal(), 'g', 10);
    }
    if (value.isDate()) {
        return QLocale::system().toString(value.toDate().toLocalTime(), QLocale::ShortFormat);
    }
    if (value.isData()) {
        const QByteArray data = value.toData();
        if (data.isEmpty()) {
            return QStringLiteral("—");
        }
        return QStringLiteral("%1 bytes").arg(data.size());
    }
    if (value.isString()) {
        if (info && info->suppressLocalization) {
            return value.toString();
        }
        return displayText(key, value.toString());
    }
    if (value.isArray()) {
        QStringList parts;
        for (const PlistValue &item : value.arrayItems()) {
            if (item.isString() || item.isInteger() || item.isReal() || item.isBoolean() || item.isDate()) {
                parts << displayValue(key, item, info);
            }
        }
        return parts.join(QStringLiteral(", "));
    }
    return QString();
}

QString LabelHumanizer::displayText(const QString &key, const QString &rawValue)
{
    const QString trimmed = rawValue.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    if (key == QLatin1String("uptime")) {
        const QString formatted = formatUptime(trimmed);
        if (!formatted.isEmpty()) {
            return formatted;
        }
    }
    if (key == QLatin1String("number_processors")) {
        const QString formatted = formatProcessorCores(trimmed);
        if (!formatted.isEmpty()) {
            return formatted;
        }
    }

    if (valueNames().contains(trimmed)) {
        return valueNames().value(trimmed);
    }

    if (looksLikeIdentifier(trimmed)) {
        QString identifier = trimmed;
        static const QStringList valuePrefixes = {
            QStringLiteral("spdisplays_"),
            QStringLiteral("sppci_"),
            QStringLiteral("spusb_"),
            QStringLiteral("coreaudio_device_type_"),
            QStringLiteral("coreaudio_"),
        };
        for (const QString &prefix : valuePrefixes) {
            if (identifier.startsWith(prefix) && identifier.size() > prefix.size()) {
                identifier = identifier.mid(prefix.size());
                break;
            }
        }
        if (valueNames().contains(identifier)) {
            return valueNames().value(identifier);
        }
        return titleCase(splitIdentifier(identifier));
    }

    return rawValue;
}

QString LabelHumanizer::formatByteSize(qint64 bytes)
{
    if (bytes < 0) {
        bytes = 0;
    }
    static const char *units[] = {"bytes", "KB", "MB", "GB", "TB", "PB"};
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1000.0 && unit < 5) {
        value /= 1000.0;
        ++unit;
    }
    if (unit == 0) {
        return QStringLiteral("%1 bytes").arg(bytes);
    }
    return QStringLiteral("%1 %2").arg(value, 0, 'f', 2).arg(QLatin1String(units[unit]));
}

QString LabelHumanizer::itemTitle(const PlistValue &item)
{
    if (!item.isDictionary()) {
        return LabelHumanizer::displayValue(QString(), item);
    }

    const QString name = item.childString(QStringLiteral("_name"));
    if (!name.isEmpty()) {
        if (keyNames().contains(name) || looksLikeIdentifier(name)) {
            return keyName(name);
        }
        return name;
    }

    if (const PlistValue *driver = item.child(QStringLiteral("Driver"))) {
        const QString title = displayValue(QStringLiteral("Driver"), *driver);
        if (!title.isEmpty()) {
            return title;
        }
    }

    const QString hardwareType = item.childString(QStringLiteral("USBKeyHardwareType"));
    if (!hardwareType.isEmpty()) {
        return displayText(QStringLiteral("USBKeyHardwareType"), hardwareType);
    }

    return QStringLiteral("Item");
}

QString LabelHumanizer::splitIdentifier(const QString &text)
{
    QString out;
    out.reserve(text.size() + 8);
    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (c == QLatin1Char('_') || c == QLatin1Char('-')) {
            if (!out.isEmpty() && !out.endsWith(QLatin1Char(' '))) {
                out += QLatin1Char(' ');
            }
            continue;
        }
        if (i > 0 && c.isUpper()) {
            const QChar prev = text.at(i - 1);
            const bool nextIsLower = (i + 1 < text.size()) && text.at(i + 1).isLower();
            if (prev.isLower() || (prev.isUpper() && nextIsLower)) {
                if (!out.endsWith(QLatin1Char(' '))) {
                    out += QLatin1Char(' ');
                }
            }
        }
        out += c;
    }
    return out.simplified();
}

QString LabelHumanizer::titleCase(const QString &text)
{
    static const QSet<QString> keepUpper = {
        QStringLiteral("USB"), QStringLiteral("UUID"), QStringLiteral("UDID"), QStringLiteral("ID"),
        QStringLiteral("MAC"), QStringLiteral("IP"), QStringLiteral("IPv4"), QStringLiteral("IPv6"),
        QStringLiteral("PCI"), QStringLiteral("GPU"), QStringLiteral("SSD"), QStringLiteral("HDD"),
        QStringLiteral("RAM"), QStringLiteral("ROM"), QStringLiteral("SMC"), QStringLiteral("BSD"),
        QStringLiteral("SATA"), QStringLiteral("NVMe"), QStringLiteral("SAS"), QStringLiteral("SPI"),
        QStringLiteral("ATA"), QStringLiteral("SCSI"), QStringLiteral("FTP"), QStringLiteral("DHCP"),
        QStringLiteral("APFS"), QStringLiteral("CPU"), QStringLiteral("OS"), QStringLiteral("VM"),
    };

    QStringList words = text.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (QString &word : words) {
        if (keepUpper.contains(word.toUpper()) || keepUpper.contains(word)) {
            word = word.toUpper();
            if (word == QLatin1String("IPV4")) {
                word = QStringLiteral("IPv4");
            } else if (word == QLatin1String("IPV6")) {
                word = QStringLiteral("IPv6");
            } else if (word == QLatin1String("NVME")) {
                word = QStringLiteral("NVMe");
            }
            continue;
        }
        if (word.size() <= 1) {
            word = word.toUpper();
            continue;
        }
        const bool hasLower = std::any_of(word.cbegin(), word.cend(), [](QChar c) { return c.isLower(); });
        if (!hasLower) {
            continue;
        }
        word[0] = word[0].toUpper();
    }
    return words.join(QLatin1Char(' '));
}

QString LabelHumanizer::stripKnownPrefix(const QString &key)
{
    for (const QString &prefix : kPrefixesToStrip) {
        if (key.startsWith(prefix) && key.size() > prefix.size()) {
            return key.mid(prefix.size());
        }
    }
    if (key.startsWith(QLatin1Char('_')) && key.size() > 1 && !key.startsWith(QLatin1String("__"))) {
        return key.mid(1);
    }
    return key;
}

bool LabelHumanizer::looksLikeIdentifier(const QString &text)
{
    if (text.isEmpty() || text.contains(QLatin1Char(' ')) || text.contains(QLatin1Char('/'))) {
        return false;
    }
    if (text.contains(QLatin1Char('.')) && text.contains(QLatin1Char('/'))) {
        return false;
    }
    static const QRegularExpression identifier(QStringLiteral("^[A-Za-z][A-Za-z0-9_-]*$"));
    return identifier.match(text).hasMatch() && (text.contains(QLatin1Char('_')) || text == text.toLower());
}
