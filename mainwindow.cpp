#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include "CLIHandler.h"
#include "DatabaseManager.h" // 为了 User 结构体
#include <QPushButton>
#include <QApplication>
#include "DocListDialog.h"
#include <QFileDialog>
#include <QDebug>
#include <QKeyEvent>
#include <QShortcut>

extern CLIHandler* g_cliHandler; // 假设有全局CLIHandler指针

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // 初始状态：只显示登录/注册页签
    ui->tabWidgetAuth->setVisible(true);
    ui->groupBoxChangePassword->setVisible(false);
    ui->btnLogout->setVisible(false);
    ui->btnViewDocs->setVisible(false);
    ui->groupBoxUsers->setVisible(false);

    // 设置登录按钮为默认按钮，这样回车键会触发它
    ui->btnLogin->setDefault(true);

    // 为登录页面的输入框安装事件过滤器
    ui->lineEditUsername->installEventFilter(this);
    ui->lineEditPassword->installEventFilter(this);

    // 为注册页面的输入框安装事件过滤器
    ui->lineEditRegisterUsername->installEventFilter(this);
    ui->lineEditRegisterPassword->installEventFilter(this);
    ui->lineEditRegisterEmail->installEventFilter(this);

    // 创建快捷键：在登录页面按回车键触发登录
    QShortcut *loginShortcut = new QShortcut(QKeySequence(Qt::Key_Return), ui->tabLogin);
    connect(loginShortcut, &QShortcut::activated, this, &MainWindow::on_btnLogin_clicked);

    QShortcut *loginShortcut2 = new QShortcut(QKeySequence(Qt::Key_Enter), ui->tabLogin);
    connect(loginShortcut2, &QShortcut::activated, this, &MainWindow::on_btnLogin_clicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        // 如果在用户名或密码输入框中按下回车键，触发登录
        if ((obj == ui->lineEditUsername || obj == ui->lineEditPassword) &&
            (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)) {

            // 只有在登录页面可见时才触发登录
            if (ui->tabWidgetAuth->isVisible() && ui->tabWidgetAuth->currentWidget() == ui->tabLogin) {
                on_btnLogin_clicked();
                return true; // 事件已处理
            }
        }

        // 如果在注册页面的输入框中按下回车键，触发注册
        if ((obj == ui->lineEditRegisterUsername || obj == ui->lineEditRegisterPassword || obj == ui->lineEditRegisterEmail) &&
            (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)) {

            // 只有在注册页面可见时才触发注册
            if (ui->tabWidgetAuth->isVisible() && ui->tabWidgetAuth->currentWidget() == ui->tabRegister) {
                on_btnRegisterUser_clicked();
                return true; // 事件已处理
            }
        }
    }

    // 调用基类的事件过滤器
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::on_btnLogin_clicked()
{
    QString username = ui->lineEditUsername->text();
    QString password = ui->lineEditPassword->text();
    if (!g_cliHandler) {
        QMessageBox::critical(this, "错误", "CLIHandler 未初始化");
        return;
    }
    qDebug() << username;
    qDebug() << password;
    try {
        std::vector<std::string> args = { "login", username.toUtf8().constData(), password.toUtf8().constData() };
        bool ok = g_cliHandler->handleLogin(args);
        if (ok) {
            QMessageBox::information(this, "登录", "登录成功");
            updateUserList();
            updateCurrentUserInfo();
        }
        else {
            QMessageBox::warning(this, "登录", "登录失败，请检查用户名和密码");
        }
    }
    catch (const std::length_error& e) {
        qDebug() << "长度错误: " << e.what();
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
        updateCurrentUserInfo();
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
        ui->tableWidgetUsers->setItem(i, 1, new QTableWidgetItem(QString::fromUtf8(user.username)));
        ui->tableWidgetUsers->setItem(i, 2, new QTableWidgetItem(QString::fromUtf8(user.email)));
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
    std::vector<User> users = g_cliHandler->getSearchedUsersForUI(keyword.toUtf8().constData());

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
        ui->tableWidgetUsers->setItem(i, 1, new QTableWidgetItem(QString::fromUtf8(user.username)));
        ui->tableWidgetUsers->setItem(i, 2, new QTableWidgetItem(QString::fromUtf8(user.email)));
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
        updateCurrentUserInfo();
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
            updateCurrentUserInfo();
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
            updateCurrentUserInfo();
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
    std::vector<std::string> args = {"register", username.toUtf8().constData(), password.toUtf8().constData(), email.toUtf8().constData()};
    bool ok = g_cliHandler->handleRegister(args);
    if (ok) {
        QMessageBox::information(this, "注册", "注册成功！");
        ui->lineEditRegisterUsername->clear();
        ui->lineEditRegisterPassword->clear();
        ui->lineEditRegisterEmail->clear();
        updateUserList();
        updateCurrentUserInfo();
    } else {
        QMessageBox::warning(this, "注册", "注册失败，请检查输入或用户名/邮箱是否已存在");
    }
}

void MainWindow::updateCurrentUserInfo()
{
    if (!g_cliHandler) {
        ui->labelCurrentUser->setText("未登录");
        ui->btnViewDocs->setVisible(false);
        // 未登录时只显示登录/注册页签
        ui->tabWidgetAuth->setVisible(true);
        ui->groupBoxChangePassword->setVisible(false);
        ui->btnLogout->setVisible(false);
        ui->groupBoxUsers->setVisible(false);
        return;
    }
    auto result = g_cliHandler->getCurrentUserForUI();
    if (result.first) {
        const User& user = result.second;
        ui->labelCurrentUser->setText(QString("用户：%1\n邮箱：%2").arg(QString::fromUtf8(user.username), QString::fromUtf8(user.email)));
        ui->btnViewDocs->setVisible(true);
        // 登录后显示功能区，隐藏登录/注册页签
        ui->tabWidgetAuth->setVisible(false);
        ui->groupBoxChangePassword->setVisible(true);
        ui->btnLogout->setVisible(true);
        ui->groupBoxUsers->setVisible(true);
    } else {
        ui->labelCurrentUser->setText("未登录");
        ui->btnViewDocs->setVisible(false);
        // 未登录时只显示登录/注册页签
        ui->tabWidgetAuth->setVisible(true);
        ui->groupBoxChangePassword->setVisible(false);
        ui->btnLogout->setVisible(false);
        ui->groupBoxUsers->setVisible(false);
    }
}

void MainWindow::on_btnChangePassword_clicked()
{
    if (!g_cliHandler) {
        QMessageBox::critical(this, "错误", "CLIHandler 未初始化");
        return;
    }
    // 检查是否已登录
    auto result = g_cliHandler->getCurrentUserForUI();
    if (!result.first) {
        QMessageBox::warning(this, "修改密码", "请先登录后再修改密码");
        return;
    }
    QString oldPass = ui->lineEditOldPassword->text();
    QString newPass = ui->lineEditNewPassword->text();
    QString confirmPass = ui->lineEditConfirmPassword->text();
    if (oldPass.isEmpty() || newPass.isEmpty() || confirmPass.isEmpty()) {
        QMessageBox::warning(this, "修改密码", "请填写完整的原密码、新密码和确认密码");
        return;
    }
    if (newPass != confirmPass) {
        QMessageBox::warning(this, "修改密码", "新密码与确认密码不一致");
        return;
    }
    std::vector<std::string> args = {"changepass", oldPass.toUtf8().constData(), newPass.toUtf8().constData()};
    bool ok = g_cliHandler->handleChangePassword(args);
    if (ok) {
        QMessageBox::information(this, "修改密码", "密码修改成功，请重新登录");
        // 清空输入框
        ui->lineEditOldPassword->clear();
        ui->lineEditNewPassword->clear();
        ui->lineEditConfirmPassword->clear();
        // 自动登出
        std::vector<std::string> logoutArgs = {"logout"};
        g_cliHandler->handleLogout(logoutArgs);
        updateUserList();
        updateCurrentUserInfo();
    } else {
        QMessageBox::warning(this, "修改密码", "密码修改失败，请检查原密码或新密码格式");
    }
}

void MainWindow::on_btnQuit_clicked()
{
    QApplication::quit();
}

void MainWindow::on_btnViewDocs_clicked()
{
    if (!g_cliHandler) return;
    auto userResult = g_cliHandler->getCurrentUserForUI();
    if (!userResult.first) return;
    int userId = userResult.second.id;
    std::vector<Document> docs = g_cliHandler->getUserDocsForUI(userId);
    DocListDialog dlg(docs, this);
    dlg.exec();
}

void MainWindow::on_btnExportUsers_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, "导出用户", "users_export.csv", "Excel Files (*.xlsx);;CSV Files (*.csv);;All Files (*)");
    if (filePath.isEmpty()) return;
    std::vector<std::string> args = { "export-users-excel", filePath.toUtf8().constData() };
    bool ok = g_cliHandler->handleExportUsersExcel(args);
    if (ok) {
        QMessageBox::information(this, "导出用户", "导出成功！");
    } else {
        QMessageBox::warning(this, "导出用户", "导出失败，请检查路径或权限");
    }
}

void MainWindow::on_btnDownloadUserTemplate_clicked()
{
    // 1. 选择保存路径
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "保存导入模板",
        "users_export.csv",
        "CSV文件 (*.csv);;所有文件 (*.*)"
    );
    if (filePath.isEmpty()) {
        return;
    }

    // 2. 下载文件
    std::string minioPath = "template/users_export.csv";
    if (!g_cliHandler || !g_cliHandler->minioClient) {
        QMessageBox::warning(this, "下载失败", "MinIO客户端未初始化");
        return;
    }
    auto result = g_cliHandler->minioClient->getObject(minioPath, filePath.toUtf8().constData());
    if (result.success && result.data.value()) {
        QMessageBox::information(this, "下载成功", "模板已保存到:\n" + filePath);
    } else {
        QMessageBox::warning(this, "下载失败", "下载过程中出现错误: " + QString::fromUtf8(result.message));
    }
}

void MainWindow::on_btnImportUsers_clicked()
{
    // 1. 选择CSV文件
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择要导入的用户CSV文件",
        "",
        "CSV文件 (*.csv);;所有文件 (*.*)"
    );
    if (filePath.isEmpty()) {
        return;
    }

    // 2. 调用导入命令
    std::vector<std::string> args = {"import-users-excel", filePath.toUtf8().constData()};
    bool ok = g_cliHandler->handleImportUsersExcel(args);
    if (ok) {
        QMessageBox::information(this, "导入成功", "用户数据导入成功！");
        updateUserList();
    } else {
        QMessageBox::warning(this, "导入失败", "用户数据导入失败，请检查文件格式或内容。");
    }
}


