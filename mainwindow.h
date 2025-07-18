#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>

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
};
#endif // MAINWINDOW_H
