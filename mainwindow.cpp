#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include "CLIHandler.h"
#include "DatabaseManager.h" // 为了 User 结构体

extern CLIHandler* g_cliHandler; // 假设有全局CLIHandler指针

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnLogin_clicked()
{
    QString username = ui->lineEditUsername->text();
    QString password = ui->lineEditPassword->text();
    if (!g_cliHandler) {
        QMessageBox::critical(this, "错误", "CLIHandler 未初始化");
        return;
    }
    std::vector<std::string> args = {"login", username.toStdString(), password.toStdString()};
    bool ok = g_cliHandler->handleLogin(args);
    if (ok) {
        QMessageBox::information(this, "登录", "登录成功");
        updateUserList();
    } else {
        QMessageBox::warning(this, "登录", "登录失败，请检查用户名和密码");
    }
}

void MainWindow::on_btnLogout_clicked()
{
    if (!g_cliHandler) {
        QMessageBox::critical(this, "错误", "CLIHandler 未初始化");
        return;
    }
    std::vector<std::string> args = {"logout"};
    bool ok = g_cliHandler->handleLogout(args);
    if (ok) {
        QMessageBox::information(this, "登出", "登出成功");
        ui->tableWidgetUsers->setRowCount(0);
    } else {
        QMessageBox::warning(this, "登出", "登出失败，当前未登录");
    }
}

void MainWindow::updateUserList()
{
    if (!g_cliHandler) {
        QMessageBox::critical(this, "错误", "CLIHandler 未初始化");
        return;
    }
    std::vector<User> users = g_cliHandler->getAllUsersForUI();

    // 设置表头
    ui->tableWidgetUsers->clear();
    ui->tableWidgetUsers->setColumnCount(4);
    QStringList headers;
    headers << "ID" << "用户名" << "邮箱" << "状态";
    ui->tableWidgetUsers->setHorizontalHeaderLabels(headers);

    // 填充数据
    ui->tableWidgetUsers->setRowCount(users.size());
    for (int i = 0; i < users.size(); ++i) {
        const User& user = users[i];
        ui->tableWidgetUsers->setItem(i, 0, new QTableWidgetItem(QString::number(user.id)));
        ui->tableWidgetUsers->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(user.username)));
        ui->tableWidgetUsers->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(user.email)));
        ui->tableWidgetUsers->setItem(i, 3, new QTableWidgetItem(user.is_active ? "激活" : "禁用"));
    }
    ui->tableWidgetUsers->resizeColumnsToContents();
}


