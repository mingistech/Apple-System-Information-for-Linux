#include "ui/MainWindow.h"

#include "model/SystemReport.h"
#include "ui/DetailView.h"
#include "ui/ReportTreeModel.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("SPX Viewer"));
    setMinimumSize(900, 560);
    resize(1180, 740);

    buildUi();
    buildMenus();
    updateWindowTitle();
}

MainWindow::~MainWindow() = default;

void MainWindow::openFile(const QString &path)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString error;
    auto report = SystemReport::load(path, &error);
    QApplication::restoreOverrideCursor();

    if (!report) {
        showOpenError(error);
        return;
    }

    m_report = std::move(report);
    m_treeModel->setReport(m_report.get());
    m_tree->expandToDepth(0);
    m_search->clear();
    m_detail->clear();
    m_stack->setCurrentWidget(m_splitter);
    m_closeAction->setEnabled(true);
    updateWindowTitle();

    if (m_treeModel->rowCount() > 0) {
        const QModelIndex first = m_treeModel->index(0, 0);
        QModelIndex toSelect = first;
        if (m_treeModel->hasChildren(first)) {
            toSelect = m_treeModel->index(0, 0, first);
        }
        m_tree->setCurrentIndex(toSelect);
        m_tree->scrollTo(toSelect);
    }

    m_statusLabel->setText(tr("Opened %1").arg(m_report->fileName()));
}

void MainWindow::openReport()
{
    QSettings settings;
    const QString lastDir = settings.value(QStringLiteral("lastOpenDirectory")).toString();
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("Open System Report"),
        lastDir,
        tr("System Reports (*.spx);;Property Lists (*.plist *.xml);;All Files (*)"));
    if (path.isEmpty()) {
        return;
    }
    settings.setValue(QStringLiteral("lastOpenDirectory"), QFileInfo(path).absolutePath());
    openFile(path);
}

void MainWindow::closeReport()
{
    m_report.reset();
    m_treeModel->clearReport();
    m_detail->clear();
    m_search->clear();
    m_stack->setCurrentWidget(m_welcome);
    m_closeAction->setEnabled(false);
    updateWindowTitle();
    m_statusLabel->setText(tr("No report open"));
}

void MainWindow::focusSearch()
{
    m_search->setFocus(Qt::ShortcutFocusReason);
    m_search->selectAll();
}

void MainWindow::onSearchTextChanged(const QString &)
{
    m_searchTimer->start();
}

void MainWindow::onCategorySelected()
{
    ReportCategory *category = m_treeModel->categoryFromIndex(m_tree->currentIndex());
    m_detail->showCategory(category);
    m_detail->applySearch(m_search->text().trimmed());
    if (category) {
        const int count = category->itemCount();
        if (count > 0) {
            m_statusLabel->setText(tr("%1 — %n item(s)", nullptr, count).arg(category->displayName));
        } else {
            m_statusLabel->setText(category->displayName);
        }
    }
}

void MainWindow::showAbout()
{
    QMessageBox::about(
        this,
        tr("About SPX Viewer"),
        tr("<h3>SPX Viewer %1</h3>"
           "<p>A Linux viewer for Apple System Information <code>.spx</code> reports.</p>"
           "<p>Open a report saved from macOS System Information "
           "(File → Save) to browse hardware, network, and software details.</p>")
            .arg(QApplication::applicationVersion()));
}

void MainWindow::buildUi()
{
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Search Report"));
    m_search->setClearButtonEnabled(true);
    m_search->setMinimumWidth(220);
    m_search->setMaximumWidth(320);

    auto *toolbar = addToolBar(tr("Search"));
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setIconSize(QSize(16, 16));
    auto *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);
    toolbar->addWidget(m_search);

    m_treeModel = new ReportTreeModel(this);
    m_tree = new QTreeView(this);
    m_tree->setModel(m_treeModel);
    m_tree->setHeaderHidden(true);
    m_tree->setUniformRowHeights(true);
    m_tree->setAnimated(true);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setIndentation(18);
    m_tree->setFrameShape(QFrame::NoFrame);

    m_detail = new DetailView(this);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->addWidget(m_tree);
    m_splitter->addWidget(m_detail);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setSizes({280, 900});

    m_welcome = new QWidget(this);
    auto *welcomeLayout = new QVBoxLayout(m_welcome);
    welcomeLayout->setContentsMargins(40, 40, 40, 40);
    auto *welcomeTitle = new QLabel(tr("Apple SPX Viewer"), m_welcome);
    QFont titleFont = welcomeTitle->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() + 8);
    titleFont.setBold(true);
    welcomeTitle->setFont(titleFont);
    welcomeTitle->setAlignment(Qt::AlignCenter);

    auto *welcomeBody = new QLabel(
        tr("Open an Apple System Information report to browse Mac hardware, "
           "network, and software details on Linux.\n\n"
           "These reports are created on a Mac with System Information → File → Save."),
        m_welcome);
    welcomeBody->setWordWrap(true);
    welcomeBody->setAlignment(Qt::AlignCenter);

    auto *openButton = new QPushButton(tr("Open SPX File…"), m_welcome);
    openButton->setMinimumHeight(36);
    openButton->setCursor(Qt::PointingHandCursor);
    connect(openButton, &QPushButton::clicked, this, &MainWindow::openReport);

    auto *buttonRow = new QHBoxLayout;
    buttonRow->addStretch();
    buttonRow->addWidget(openButton);
    buttonRow->addStretch();

    welcomeLayout->addStretch();
    welcomeLayout->addWidget(welcomeTitle);
    welcomeLayout->addSpacing(8);
    welcomeLayout->addWidget(welcomeBody);
    welcomeLayout->addSpacing(20);
    welcomeLayout->addLayout(buttonRow);
    welcomeLayout->addStretch();

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_welcome);
    m_stack->addWidget(m_splitter);
    setCentralWidget(m_stack);

    m_statusLabel = new QLabel(tr("No report open"), this);
    statusBar()->addWidget(m_statusLabel, 1);

    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(120);

    connect(m_search, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_searchTimer, &QTimer::timeout, this, &MainWindow::applyTreeFilter);
    connect(m_tree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MainWindow::onCategorySelected);
}

void MainWindow::buildMenus()
{
    auto *fileMenu = menuBar()->addMenu(tr("&File"));

    auto *openAction = fileMenu->addAction(tr("&Open…"));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openReport);

    m_closeAction = fileMenu->addAction(tr("&Close Report"));
    m_closeAction->setEnabled(false);
    connect(m_closeAction, &QAction::triggered, this, &MainWindow::closeReport);

    fileMenu->addSeparator();

    auto *quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    auto *editMenu = menuBar()->addMenu(tr("&Edit"));
    auto *findAction = editMenu->addAction(tr("&Find"));
    findAction->setShortcut(QKeySequence::Find);
    connect(findAction, &QAction::triggered, this, &MainWindow::focusSearch);

    auto *helpMenu = menuBar()->addMenu(tr("&Help"));
    auto *aboutAction = helpMenu->addAction(tr("&About"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::updateWindowTitle()
{
    if (m_report) {
        setWindowTitle(tr("%1 — SPX Viewer").arg(m_report->fileName()));
    } else {
        setWindowTitle(tr("SPX Viewer"));
    }
}

void MainWindow::applyTreeFilter()
{
    const QString query = m_search->text().trimmed();
    const QModelIndex root;
    const int rows = m_treeModel->rowCount(root);
    for (int row = 0; row < rows; ++row) {
        filterTreeItem(m_treeModel->index(row, 0, root), query);
    }
    m_detail->applySearch(query);
}

bool MainWindow::filterTreeItem(const QModelIndex &index, const QString &query)
{
    bool childMatch = false;
    const int rows = m_treeModel->rowCount(index);
    for (int row = 0; row < rows; ++row) {
        childMatch = filterTreeItem(m_treeModel->index(row, 0, index), query) || childMatch;
    }

    bool selfMatch = query.isEmpty();
    if (!query.isEmpty()) {
        ReportCategory *category = m_treeModel->categoryFromIndex(index);
        selfMatch = category && category->matches(query);
    }

    const bool visible = selfMatch || childMatch || query.isEmpty();
    m_tree->setRowHidden(index.row(), index.parent(), !visible);
    if (childMatch && !query.isEmpty()) {
        m_tree->expand(index);
    }
    return visible;
}

void MainWindow::showOpenError(const QString &detail)
{
    QString text = tr("The selected file does not appear to be a valid Apple System Information SPX report.");
    if (!detail.isEmpty() && !detail.contains(QLatin1String("does not appear to be a valid"))) {
        text += QStringLiteral("\n\n") + detail;
    }
    QMessageBox::critical(this, tr("Unable to Open System Report"), text);
}
