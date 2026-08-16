#include "model/SystemReport.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFileInfo>
#include <QTextStream>
#include <cstdio>

static int dumpReport(const QString &path)
{
    QString error;
    const auto report = SystemReport::load(path, &error);
    if (!report) {
        QTextStream(stderr) << "Failed to open report: " << error << '\n';
        return 1;
    }

    QTextStream out(stdout);
    out << report->fileName() << '\n';

    const auto printNode = [&](const auto &self, const ReportCategory *node, int depth) -> void {
        out << QString(depth * 2, QLatin1Char(' ')) << node->displayName
            << " [" << node->dataType << "] (" << node->itemCount() << " items)\n";
        for (const ReportCategory *child : node->children) {
            self(self, child, depth + 1);
        }
    };
    for (const ReportCategory *root : report->rootCategories()) {
        printNode(printNode, root, 0);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc >= 3 && QString::fromLocal8Bit(argv[1]) == QLatin1String("--dump-tree")) {
        QCoreApplication app(argc, argv);
        return dumpReport(QFileInfo(QString::fromLocal8Bit(argv[2])).absoluteFilePath());
    }

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("SPX Viewer"));
    app.setApplicationDisplayName(QStringLiteral("SPX Viewer"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setOrganizationName(QStringLiteral("SPX Viewer"));
    app.setDesktopFileName(QStringLiteral("io.github.mingistech.SpxViewer"));

    MainWindow window;
    window.show();

    if (argc > 1) {
        const QString path = QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();
        if (QFileInfo::exists(path)) {
            window.openFile(path);
        }
    }

    return app.exec();
}
