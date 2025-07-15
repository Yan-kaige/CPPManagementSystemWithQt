#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include "CLIHandler.h"
#include "DatabaseManager.h" // 为了 User 结构体
#include <QPushButton>

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
    ui->tableWidgetUsers->setColumnCount(6);
    QStringList headers;
    headers << "ID" << "用户名" << "邮箱" << "状态" << "操作" << "状态操作";
    ui->tableWidgetUsers->setHorizontalHeaderLabels(headers);

    // 填充数据
    ui->tableWidgetUsers->setRowCount(users.size());
    for (int i = 0; i < users.size(); ++i) {
        const User& user = users[i];
        ui->tableWidgetUsers->setItem(i, 0, new QTableWidgetItem(QString::number(user.id)));
        ui->tableWidgetUsers->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(user.username)));
        ui->tableWidgetUsers->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(user.email)));
        ui->tableWidgetUsers->setItem(i, 3, new QTableWidgetItem(user.is_active ? "激活" : "禁用"));
        QPushButton* btnDel = new QPushButton("删除");
        btnDel->setProperty("userId", user.id);
        connect(btnDel, &QPushButton::clicked, this, &MainWindow::onDeleteUserClicked);
        ui->tableWidgetUsers->setCellWidget(i, 4, btnDel);
        QPushButton* btnToggle = new QPushButton(user.is_active ? "禁用" : "激活");
        btnToggle->setProperty("userId", user.id);
        btnToggle->setProperty("isActive", user.is_active);
        connect(btnToggle, &QPushButton::clicked, this, &MainWindow::onToggleUserActiveClicked);
        ui->tableWidgetUsers->setCellWidget(i, 5, btnToggle);
    }
    ui->tableWidgetUsers->resizeColumnsToContents();
}

void MainWindow::on_btnSearchUser_clicked()
{
    if (!g_cliHandler) {
        QMessageBox::critical(this, "错误", "CLIHandler 未初始化");
        return;
    }
    QString keyword = ui->lineEditSearchUser->text().trimmed();
    if (keyword.isEmpty()) {
        updateUserList();
        return;
    }
    std::vector<User> users = g_cliHandler->getSearchedUsersForUI(keyword.toStdString());

    // 设置表头
    ui->tableWidgetUsers->clear();
    ui->tableWidgetUsers->setColumnCount(6);
    QStringList headers;
    headers << "ID" << "用户名" << "邮箱" << "状态" << "操作" << "状态操作";
    ui->tableWidgetUsers->setHorizontalHeaderLabels(headers);

    // 填充数据
    ui->tableWidgetUsers->setRowCount(users.size());
    for (int i = 0; i < users.size(); ++i) {
        const User& user = users[i];
        ui->tableWidgetUsers->setItem(i, 0, new QTableWidgetItem(QString::number(user.id)));
        ui->tableWidgetUsers->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(user.username)));
        ui->tableWidgetUsers->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(user.email)));
        ui->tableWidgetUsers->setItem(i, 3, new QTableWidgetItem(user.is_active ? "激活" : "禁用"));
        QPushButton* btnDel = new QPushButton("删除");
        btnDel->setProperty("userId", user.id);
        connect(btnDel, &QPushButton::clicked, this, &MainWindow::onDeleteUserClicked);
        ui->tableWidgetUsers->setCellWidget(i, 4, btnDel);
        QPushButton* btnToggle = new QPushButton(user.is_active ? "禁用" : "激活");
        btnToggle->setProperty("userId", user.id);
        btnToggle->setProperty("isActive", user.is_active);
        connect(btnToggle, &QPushButton::clicked, this, &MainWindow::onToggleUserActiveClicked);
        ui->tableWidgetUsers->setCellWidget(i, 5, btnToggle);
    }
    ui->tableWidgetUsers->resizeColumnsToContents();
}

void MainWindow::onDeleteUserClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    int userId = btn->property("userId").toInt();
    if (QMessageBox::question(this, "确认删除", QString("确定要删除用户ID %1 吗？").arg(userId)) != QMessageBox::Yes) {
        return;
    }
    if (!g_cliHandler) {
        QMessageBox::critical(this, "错误", "CLIHandler 未初始化");
        return;
    }
    std::vector<std::string> args = {"deleteuser", std::to_string(userId)};
    bool ok = g_cliHandler->handleDeleteUser(args);
    if (ok) {
        QMessageBox::information(this, "删除用户", "删除成功！");
        updateUserList();
    } else {
        QMessageBox::warning(this, "删除用户", "删除失败，不能删除当前登录用户或用户不存在");
    }
}

void MainWindow::onToggleUserActiveClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    int userId = btn->property("userId").toInt();
    bool isActive = btn->property("isActive").toBool();
    if (!g_cliHandler) {
        QMessageBox::critical(this, "错误", "CLIHandler 未初始化");
        return;
    }
    std::vector<std::string> args;
    if (isActive) {
        if (QMessageBox::question(this, "确认禁用", QString("确定要禁用用户ID %1 吗？").arg(userId)) != QMessageBox::Yes) {
            return;
        }
        args = {"deactivate", std::to_string(userId)};
        bool ok = g_cliHandler->handleDeactivateUser(args);
        if (ok) {
            QMessageBox::information(this, "禁用用户", "禁用成功！");
            updateUserList();
        } else {
            QMessageBox::warning(this, "禁用用户", "禁用失败");
        }
    } else {
        if (QMessageBox::question(this, "确认激活", QString("确定要激活用户ID %1 吗？").arg(userId)) != QMessageBox::Yes) {
            return;
        }
        args = {"activate", std::to_string(userId)};
        bool ok = g_cliHandler->handleActivateUser(args);
        if (ok) {
            QMessageBox::information(this, "激活用户", "激活成功！");
            updateUserList();
        } else {
            QMessageBox::warning(this, "激活用户", "激活失败");
        }
    }
}

void MainWindow::on_btnRegisterUser_clicked()
{
    if (!g_cliHandler) {
        QMessageBox::critical(this, "错误", "CLIHandler 未初始化");
        return;
    }
    QString username = ui->lineEditRegisterUsername->text().trimmed();
    QString password = ui->lineEditRegisterPassword->text();
    QString email = ui->lineEditRegisterEmail->text().trimmed();
    if (username.isEmpty() || password.isEmpty() || email.isEmpty()) {
        QMessageBox::warning(this, "注册", "请填写完整的用户名、密码和邮箱");
        return;
    }
    std::vector<std::string> args = {"register", username.toStdString(), password.toStdString(), email.toStdString()};
    bool ok = g_cliHandler->handleRegister(args);
    if (ok) {
        QMessageBox::information(this, "注册", "注册成功！");
        ui->lineEditRegisterUsername->clear();
        ui->lineEditRegisterPassword->clear();
        ui->lineEditRegisterEmail->clear();
        updateUserList();
    } else {
        QMessageBox::warning(this, "注册", "注册失败，请检查输入或用户名/邮箱是否已存在");
    }
}


