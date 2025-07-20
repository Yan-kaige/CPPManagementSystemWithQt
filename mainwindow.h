#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QStackedWidget>
#include <QSplitter>
#include "common.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::MainWindow *ui;

    // 新的UI组件
    QTreeWidget *treeWidgetMenu;
    QStackedWidget *stackedWidgetContent;
    QWidget *pageUserManagement;
    QWidget *pageDocumentManagement;
    QWidget *pageSharedDocuments;

    // 初始化方法
    void setupNewUI();
    void setupMenuTree();
    void setupContentPages();
    void showUserManagementPage();
    void showDocumentManagementPage();
    void showSharedDocumentsPage();
    void updateDocumentList();
    void updateSharedDocumentList();
    void onUploadDocumentClicked();
    void onSearchDocumentsClicked();
    void onResetDocumentsClicked();
    void onExportDocumentsClicked();
    void onSearchSharedDocumentsClicked();
    void onResetSharedDocumentsClicked();
    void updateDocumentTableWithData(QTableWidget *table, const std::vector<Document>& docs);
    void updateSharedDocumentTableWithData(QTableWidget *table, const std::vector<Document>& docs);

    void onSearchUsersClicked();
    void onResetUsersClicked();
    void onAddUserClicked();
    void onExportUsersClicked();
    void updateUserTableWithData(QTableWidget *table, const std::vector<User>& users);


private slots:
    void on_btnLogin_clicked();
    void on_btnLogout_clicked();
    void updateUserList();
    void on_btnSearchUser_clicked();
    void on_btnRegisterUser_clicked();
    void onDeleteUserClicked();
    void onToggleUserActiveClicked();
    void updateCurrentUserInfo();
    void on_btnChangePasswordDialog_clicked();
    void on_btnQuit_clicked();
    void on_btnViewDocs_clicked();
    void on_btnViewSharedDocs_clicked();
    void on_btnExportUsers_clicked();
    void on_btnDownloadUserTemplate_clicked();
    void on_btnImportUsers_clicked();

    // 新的槽函数
    void onMenuItemClicked(QTreeWidgetItem *item, int column);

    // 权限控制方法
    void updateMenuPermissions();
    bool hasPermission(const QString& permission);
};
#endif // MAINWINDOW_H
