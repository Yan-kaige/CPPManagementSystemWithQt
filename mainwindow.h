#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QLabel>
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

    // 验证码相关组件 - 修复编译错误
    QLineEdit *loginCaptchaEdit;
    QLineEdit *registerCaptchaEdit;
    QLabel *loginCaptchaLabel;
    QLabel *registerCaptchaLabel;
    QPushButton *loginRefreshCaptchaBtn;
    QPushButton *registerRefreshCaptchaBtn;
    QLabel *loginCaptchaLabelText;  // "验证码:" 标签
    QLabel *registerCaptchaLabelText;  // "验证码:" 标签
    QString loginCaptchaCode;
    QString registerCaptchaCode;
    int loginFailCount;
    int registerFailCount;

    // 初始化方法
    void setupNewUI();
    void setupMenuTree();
    void setupContentPages();
    void showUserManagementPage();
    void showDocumentManagementPage();
    void showSharedDocumentsPage();
    void showPermissionManagementDialog();
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
    
    // 辅助方法
    void showErrorDialog(const QString& title, const QString& message);
    
    // 验证码相关方法
    QString generateCaptchaCode();
    void showLoginCaptcha();
    void showRegisterCaptcha();
    void hideLoginCaptcha();
    void hideRegisterCaptcha();
    void refreshLoginCaptcha();
    void refreshRegisterCaptcha();
};
#endif // MAINWINDOW_H
