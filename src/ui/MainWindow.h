#pragma once

#include <QMainWindow>
#include <memory>

class DetailView;
class ReportTreeModel;
class SystemReport;
class QLabel;
class QLineEdit;
class QSplitter;
class QStackedWidget;
class QTimer;
class QTreeView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void openFile(const QString &path);

private slots:
    void openReport();
    void closeReport();
    void focusSearch();
    void onSearchTextChanged(const QString &text);
    void onCategorySelected();
    void showAbout();

private:
    void buildUi();
    void buildMenus();
    void updateWindowTitle();
    void applyTreeFilter();
    bool filterTreeItem(const QModelIndex &index, const QString &query);
    void showOpenError(const QString &detail);

    QStackedWidget *m_stack = nullptr;
    QWidget *m_welcome = nullptr;
    QSplitter *m_splitter = nullptr;
    QTreeView *m_tree = nullptr;
    DetailView *m_detail = nullptr;
    ReportTreeModel *m_treeModel = nullptr;
    QLineEdit *m_search = nullptr;
    QLabel *m_statusLabel = nullptr;
    QAction *m_closeAction = nullptr;
    QTimer *m_searchTimer = nullptr;
    std::unique_ptr<SystemReport> m_report;
};
