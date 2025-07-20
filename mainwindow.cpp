#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include "CLIHandler.h"
#include "DatabaseManager.h" // 为了 User 结构体
#include <QPushButton>
#include <QApplication>
#include "DocListDialog.h"
#include "ChangePasswordDialog.h"
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

    // 设置新的UI布局
    setupNewUI();

    // 初始化菜单树
    setupMenuTree();

    // 初始化内容页面
    setupContentPages();

    // 为登录页面的输入框安装事件过滤器
    QLineEdit *loginUsername = findChild<QLineEdit*>("lineEditLoginUsername");
    QLineEdit *loginPassword = findChild<QLineEdit*>("lineEditLoginPassword");
    if (loginUsername) loginUsername->installEventFilter(this);
    if (loginPassword) loginPassword->installEventFilter(this);

    // 为注册页面的输入框安装事件过滤器
    QLineEdit *registerUsername = findChild<QLineEdit*>("lineEditRegisterUsername");
    QLineEdit *registerPassword = findChild<QLineEdit*>("lineEditRegisterPassword");
    QLineEdit *registerEmail = findChild<QLineEdit*>("lineEditRegisterEmail");
    if (registerUsername) registerUsername->installEventFilter(this);
    if (registerPassword) registerPassword->installEventFilter(this);
    if (registerEmail) registerEmail->installEventFilter(this);

    // 注意：快捷键功能已通过事件过滤器实现，这里不再需要单独的快捷键

    // 初始化用户界面状态
    updateCurrentUserInfo();
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        // 查找动态创建的组件
        QLineEdit *loginUsername = findChild<QLineEdit*>("lineEditLoginUsername");
        QLineEdit *loginPassword = findChild<QLineEdit*>("lineEditLoginPassword");
        QLineEdit *registerUsername = findChild<QLineEdit*>("lineEditRegisterUsername");
        QLineEdit *registerPassword = findChild<QLineEdit*>("lineEditRegisterPassword");
        QLineEdit *registerEmail = findChild<QLineEdit*>("lineEditRegisterEmail");
        QTabWidget *tabAuth = findChild<QTabWidget*>("tabWidgetAuth");

        // 如果在用户名或密码输入框中按下回车键，触发登录
        if ((obj == loginUsername || obj == loginPassword) &&
            (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)) {

            // 只有在登录页面可见时才触发登录
            if (tabAuth && tabAuth->isVisible() && tabAuth->currentIndex() == 0) { // 登录是第一个标签页
                on_btnLogin_clicked();
                return true; // 事件已处理
            }
        }

        // 如果在注册页面的输入框中按下回车键，触发注册
        if ((obj == registerUsername || obj == registerPassword || obj == registerEmail) &&
            (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)) {

            // 只有在注册页面可见时才触发注册
            if (tabAuth && tabAuth->isVisible() && tabAuth->currentIndex() == 1) { // 注册是第二个标签页
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
    // 查找动态创建的登录输入框
    QLineEdit *usernameEdit = findChild<QLineEdit*>("lineEditLoginUsername");
    QLineEdit *passwordEdit = findChild<QLineEdit*>("lineEditLoginPassword");

    if (!usernameEdit || !passwordEdit) {
        QMessageBox::critical(this, "错误", "UI组件未找到");
        return;
    }

    QString username = usernameEdit->text();
    QString password = passwordEdit->text();

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
            // 恢复欢迎页面的原始提示文字
            QLabel *welcomeDesc = findChild<QLabel*>("labelWelcomeDesc");
            if (welcomeDesc) {
                welcomeDesc->setText("请先登录以使用系统功能");
            }
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
        // 清空用户表格
        QTableWidget *tableUsers = findChild<QTableWidget*>("tableWidgetUsers");
        if (tableUsers) {
            tableUsers->setRowCount(0);
        }
        // 切换回欢迎页面并更新提示文字
        if (stackedWidgetContent) {
            stackedWidgetContent->setCurrentIndex(0); // 欢迎页面是第一个页面

            // 更新欢迎页面的提示文字
            QLabel *welcomeDesc = findChild<QLabel*>("labelWelcomeDesc");
            if (welcomeDesc) {
                welcomeDesc->setText("您已成功登出，请重新登录以使用系统功能");
            }
        }
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

    // 查找动态创建的用户表格
    QTableWidget *tableUsers = findChild<QTableWidget*>("tableWidgetUsers");
    if (!tableUsers) {
        qDebug() << "用户表格未找到";
        return;
    }

    std::vector<User> users = g_cliHandler->getAllUsersForUI();

    // 设置表头
    tableUsers->clear();
    tableUsers->setColumnCount(6);
    QStringList headers;
    headers << "ID" << "用户名" << "邮箱" << "状态" << "操作" << "状态操作";
    tableUsers->setHorizontalHeaderLabels(headers);

    // 填充数据
    tableUsers->setRowCount(users.size());
    for (int i = 0; i < users.size(); ++i) {
        const User& user = users[i];
        tableUsers->setItem(i, 0, new QTableWidgetItem(QString::number(user.id)));
        tableUsers->setItem(i, 1, new QTableWidgetItem(QString::fromUtf8(user.username)));
        tableUsers->setItem(i, 2, new QTableWidgetItem(QString::fromUtf8(user.email)));
        tableUsers->setItem(i, 3, new QTableWidgetItem(user.is_active ? "激活" : "禁用"));
        QPushButton* btnDel = new QPushButton("删除");
        btnDel->setProperty("userId", user.id);
        connect(btnDel, &QPushButton::clicked, this, &MainWindow::onDeleteUserClicked);
        tableUsers->setCellWidget(i, 4, btnDel);
        QPushButton* btnToggle = new QPushButton(user.is_active ? "禁用" : "激活");
        btnToggle->setProperty("userId", user.id);
        btnToggle->setProperty("isActive", user.is_active);
        connect(btnToggle, &QPushButton::clicked, this, &MainWindow::onToggleUserActiveClicked);
        tableUsers->setCellWidget(i, 5, btnToggle);
    }
    tableUsers->resizeColumnsToContents();
}

void MainWindow::on_btnSearchUser_clicked()
{
    if (!g_cliHandler) {
        QMessageBox::critical(this, "错误", "CLIHandler 未初始化");
        return;
    }

    // 查找动态创建的搜索输入框和用户表格
    QLineEdit *searchEdit = findChild<QLineEdit*>("lineEditSearchUser");
    QTableWidget *tableUsers = findChild<QTableWidget*>("tableWidgetUsers");

    if (!searchEdit || !tableUsers) {
        QMessageBox::critical(this, "错误", "UI组件未找到");
        return;
    }

    QString keyword = searchEdit->text().trimmed();
    if (keyword.isEmpty()) {
        updateUserList();
        return;
    }
    std::vector<User> users = g_cliHandler->getSearchedUsersForUI(keyword.toUtf8().constData());

    // 设置表头
    tableUsers->clear();
    tableUsers->setColumnCount(6);
    QStringList headers;
    headers << "ID" << "用户名" << "邮箱" << "状态" << "操作" << "状态操作";
    tableUsers->setHorizontalHeaderLabels(headers);

    // 填充数据
    tableUsers->setRowCount(users.size());
    for (int i = 0; i < users.size(); ++i) {
        const User& user = users[i];
        tableUsers->setItem(i, 0, new QTableWidgetItem(QString::number(user.id)));
        tableUsers->setItem(i, 1, new QTableWidgetItem(QString::fromUtf8(user.username)));
        tableUsers->setItem(i, 2, new QTableWidgetItem(QString::fromUtf8(user.email)));
        tableUsers->setItem(i, 3, new QTableWidgetItem(user.is_active ? "激活" : "禁用"));
        QPushButton* btnDel = new QPushButton("删除");
        btnDel->setProperty("userId", user.id);
        connect(btnDel, &QPushButton::clicked, this, &MainWindow::onDeleteUserClicked);
        tableUsers->setCellWidget(i, 4, btnDel);
        QPushButton* btnToggle = new QPushButton(user.is_active ? "禁用" : "激活");
        btnToggle->setProperty("userId", user.id);
        btnToggle->setProperty("isActive", user.is_active);
        connect(btnToggle, &QPushButton::clicked, this, &MainWindow::onToggleUserActiveClicked);
        tableUsers->setCellWidget(i, 5, btnToggle);
    }
    tableUsers->resizeColumnsToContents();
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

    // 查找动态创建的注册输入框
    QLineEdit *usernameEdit = findChild<QLineEdit*>("lineEditRegisterUsername");
    QLineEdit *passwordEdit = findChild<QLineEdit*>("lineEditRegisterPassword");
    QLineEdit *emailEdit = findChild<QLineEdit*>("lineEditRegisterEmail");

    if (!usernameEdit || !passwordEdit || !emailEdit) {
        QMessageBox::critical(this, "错误", "UI组件未找到");
        return;
    }

    QString username = usernameEdit->text().trimmed();
    QString password = passwordEdit->text();
    QString email = emailEdit->text().trimmed();

    if (username.isEmpty() || password.isEmpty() || email.isEmpty()) {
        QMessageBox::warning(this, "注册", "请填写完整的用户名、密码和邮箱");
        return;
    }

    std::vector<std::string> args = {"register", username.toUtf8().constData(), password.toUtf8().constData(), email.toUtf8().constData()};
    bool ok = g_cliHandler->handleRegister(args);
    if (ok) {
        QMessageBox::information(this, "注册", "注册成功！");
        usernameEdit->clear();
        passwordEdit->clear();
        emailEdit->clear();
        updateUserList();
        updateCurrentUserInfo();
    } else {
        QMessageBox::warning(this, "注册", "注册失败，请检查输入或用户名/邮箱是否已存在");
    }
}

void MainWindow::updateCurrentUserInfo()
{
    // 查找动态创建的UI组件
    QLabel *labelCurrentUser = findChild<QLabel*>("labelCurrentUser");
    QTabWidget *tabAuth = findChild<QTabWidget*>("tabWidgetAuth");
    QPushButton *btnChangePassword = findChild<QPushButton*>("btnChangePasswordDialog");
    QPushButton *btnLogout = findChild<QPushButton*>("btnLogout");

    if (!g_cliHandler) {
        if (labelCurrentUser) labelCurrentUser->setText("未登录");
        if (tabAuth) tabAuth->setVisible(true);
        if (btnChangePassword) btnChangePassword->setVisible(false);
        if (btnLogout) btnLogout->setVisible(false);
        if (treeWidgetMenu) treeWidgetMenu->setVisible(false);
        // 切换到欢迎页面
        if (stackedWidgetContent) {
            stackedWidgetContent->setCurrentIndex(0);
        }
        return;
    }

    auto result = g_cliHandler->getCurrentUserForUI();
    if (result.first) {
        const User& user = result.second;
        if (labelCurrentUser) {
            labelCurrentUser->setText(QString("用户：%1\n邮箱：%2")
                .arg(QString::fromUtf8(user.username), QString::fromUtf8(user.email)));
        }

        // 登录后显示功能菜单，隐藏登录/注册页签
        if (tabAuth) tabAuth->setVisible(false);
        if (btnChangePassword) btnChangePassword->setVisible(true);
        if (btnLogout) btnLogout->setVisible(true);
        if (treeWidgetMenu) treeWidgetMenu->setVisible(true);

        // 更新菜单权限
        updateMenuPermissions();

    } else {
        if (labelCurrentUser) labelCurrentUser->setText("未登录");
        if (tabAuth) tabAuth->setVisible(true);
        if (btnChangePassword) btnChangePassword->setVisible(false);
        if (btnLogout) btnLogout->setVisible(false);
        if (treeWidgetMenu) treeWidgetMenu->setVisible(false);

        // 切换到欢迎页面
        if (stackedWidgetContent) {
            stackedWidgetContent->setCurrentIndex(0);
        }

        // 更新菜单权限
        updateMenuPermissions();
    }
}

void MainWindow::on_btnChangePasswordDialog_clicked()
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

    // 创建并显示修改密码对话框
    ChangePasswordDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString oldPass = dialog.getOldPassword();
        QString newPass = dialog.getNewPassword();

        std::vector<std::string> args = {"changepass", oldPass.toUtf8().constData(), newPass.toUtf8().constData()};
        bool ok = g_cliHandler->handleChangePassword(args);

        if (ok) {
            QMessageBox::information(this, "修改密码", "密码修改成功，请重新登录");
            // 自动登出
            std::vector<std::string> logoutArgs = {"logout"};
            g_cliHandler->handleLogout(logoutArgs);
            updateUserList();
            updateCurrentUserInfo();
        } else {
            QMessageBox::warning(this, "修改密码", "密码修改失败，请检查原密码或新密码格式");
        }
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

void MainWindow::on_btnViewSharedDocs_clicked()
{
    if (!g_cliHandler) return;
    auto userResult = g_cliHandler->getCurrentUserForUI();
    if (!userResult.first) {
        QMessageBox::warning(this, "查看分享文档", "请先登录！");
        return;
    }
    int userId = userResult.second.id;
    std::vector<Document> docs = g_cliHandler->getSharedDocsForUI(userId);
    DocListDialog dlg(docs, this);
    dlg.setWindowTitle("分享给我的文档");
    dlg.exec();
}

void MainWindow::setupNewUI()
{
    // 清空中央窗口
    QWidget *centralWidget = ui->centralwidget;
    if (centralWidget->layout()) {
        delete centralWidget->layout();
    }

    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // 创建顶部工具栏
    QHBoxLayout *topLayout = new QHBoxLayout();

    // 添加弹性空间
    topLayout->addStretch();

    // 创建用户信息标签
    QLabel *labelCurrentUser = new QLabel("未登录", this);
    labelCurrentUser->setObjectName("labelCurrentUser");
    labelCurrentUser->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    topLayout->addWidget(labelCurrentUser);

    // 创建修改密码按钮
    QPushButton *btnChangePassword = new QPushButton("修改密码", this);
    btnChangePassword->setObjectName("btnChangePasswordDialog");
    btnChangePassword->setVisible(false);
    topLayout->addWidget(btnChangePassword);

    // 创建登出按钮
    QPushButton *btnLogout = new QPushButton("登出", this);
    btnLogout->setObjectName("btnLogout");
    btnLogout->setVisible(false);
    topLayout->addWidget(btnLogout);

    // 创建退出按钮
    QPushButton *btnQuit = new QPushButton("退出", this);
    btnQuit->setObjectName("btnQuit");
    topLayout->addWidget(btnQuit);

    mainLayout->addLayout(topLayout);

    // 创建分割器
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setStyleSheet("QSplitter::handle { background-color: #d0d0d0; width: 2px; }");

    // 创建左侧面板
    QWidget *leftPanel = new QWidget(this);
    leftPanel->setMinimumWidth(280);
    leftPanel->setMaximumWidth(380);
    leftPanel->setStyleSheet("QWidget { background-color: #f8f9fa; border-right: 1px solid #e0e0e0; }");

    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setSpacing(10);
    leftLayout->setContentsMargins(10, 10, 10, 10);

    // 添加顶部弹性空间（未登录时居中显示登录界面）
    leftLayout->addStretch(1);

    // 创建登录/注册标签页
    QTabWidget *tabAuth = new QTabWidget(this);
    tabAuth->setObjectName("tabWidgetAuth");
    tabAuth->setStyleSheet(
        "QTabWidget::pane { "
            "border: 1px solid #d0d0d0; "
            "border-radius: 8px; "
            "background-color: white; "
            "margin-top: -1px; "
        "}"
        "QTabBar::tab { "
            "background: #f5f5f5; "
            "border: 1px solid #d0d0d0; "
            "padding: 10px 20px; "
            "margin-right: 2px; "
            "border-bottom: none; "
            "border-top-left-radius: 6px; "
            "border-top-right-radius: 6px; "
            "color: #666; "
            "font-weight: 500; "
        "}"
        "QTabBar::tab:selected { "
            "background: white; "
            "color: #333; "
            "border-bottom: 1px solid white; "
            "font-weight: bold; "
        "}"
        "QTabBar::tab:hover:!selected { "
            "background: #e8e8e8; "
            "color: #333; "
        "}"
        "QTabWidget QWidget { "
            "border: none; "
        "}"
    );

    // 登录页面
    QWidget *loginTab = new QWidget();
    loginTab->setStyleSheet("QWidget { background-color: white; border: none; }");
    QVBoxLayout *loginLayout = new QVBoxLayout(loginTab);
    loginLayout->setSpacing(15);
    loginLayout->setContentsMargins(20, 30, 20, 30);

    // 添加顶部间距
    loginLayout->addStretch(1);

    // 用户名行
    QHBoxLayout *usernameLayout = new QHBoxLayout();
    QLabel *usernameLabel = new QLabel("用户名:", this);
    usernameLabel->setMinimumWidth(60);
    usernameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QLineEdit *loginUsername = new QLineEdit(this);
    loginUsername->setObjectName("lineEditLoginUsername");
    loginUsername->setMinimumHeight(30);
    loginUsername->setPlaceholderText("请输入用户名");
    usernameLayout->addWidget(usernameLabel);
    usernameLayout->addWidget(loginUsername);
    loginLayout->addLayout(usernameLayout);

    // 密码行
    QHBoxLayout *passwordLayout = new QHBoxLayout();
    QLabel *passwordLabel = new QLabel("密码:", this);
    passwordLabel->setMinimumWidth(60);
    passwordLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QLineEdit *loginPassword = new QLineEdit(this);
    loginPassword->setObjectName("lineEditLoginPassword");
    loginPassword->setEchoMode(QLineEdit::Password);
    loginPassword->setMinimumHeight(30);
    loginPassword->setPlaceholderText("请输入密码");
    passwordLayout->addWidget(passwordLabel);
    passwordLayout->addWidget(loginPassword);
    loginLayout->addLayout(passwordLayout);

    // 登录按钮
    QPushButton *btnLogin = new QPushButton("登录", this);
    btnLogin->setObjectName("btnLogin");
    btnLogin->setDefault(true);
    btnLogin->setMinimumHeight(35);
    btnLogin->setStyleSheet("QPushButton { font-size: 14px; font-weight: bold; }");
    loginLayout->addWidget(btnLogin);

    // 添加底部间距
    loginLayout->addStretch(1);

    tabAuth->addTab(loginTab, "登录");

    // 注册页面
    QWidget *registerTab = new QWidget();
    registerTab->setStyleSheet("QWidget { background-color: white; border: none; }");
    QVBoxLayout *registerLayout = new QVBoxLayout(registerTab);
    registerLayout->setSpacing(15);
    registerLayout->setContentsMargins(20, 30, 20, 30);

    // 添加顶部间距
    registerLayout->addStretch(1);

    // 用户名行
    QHBoxLayout *regUsernameLayout = new QHBoxLayout();
    QLabel *regUsernameLabel = new QLabel("用户名:", this);
    regUsernameLabel->setMinimumWidth(60);
    regUsernameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QLineEdit *registerUsername = new QLineEdit(this);
    registerUsername->setObjectName("lineEditRegisterUsername");
    registerUsername->setMinimumHeight(30);
    registerUsername->setPlaceholderText("请输入用户名");
    regUsernameLayout->addWidget(regUsernameLabel);
    regUsernameLayout->addWidget(registerUsername);
    registerLayout->addLayout(regUsernameLayout);

    // 密码行
    QHBoxLayout *regPasswordLayout = new QHBoxLayout();
    QLabel *regPasswordLabel = new QLabel("密码:", this);
    regPasswordLabel->setMinimumWidth(60);
    regPasswordLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QLineEdit *registerPassword = new QLineEdit(this);
    registerPassword->setObjectName("lineEditRegisterPassword");
    registerPassword->setEchoMode(QLineEdit::Password);
    registerPassword->setMinimumHeight(30);
    registerPassword->setPlaceholderText("请输入密码");
    regPasswordLayout->addWidget(regPasswordLabel);
    regPasswordLayout->addWidget(registerPassword);
    registerLayout->addLayout(regPasswordLayout);

    // 邮箱行
    QHBoxLayout *regEmailLayout = new QHBoxLayout();
    QLabel *regEmailLabel = new QLabel("邮箱:", this);
    regEmailLabel->setMinimumWidth(60);
    regEmailLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QLineEdit *registerEmail = new QLineEdit(this);
    registerEmail->setObjectName("lineEditRegisterEmail");
    registerEmail->setMinimumHeight(30);
    registerEmail->setPlaceholderText("请输入邮箱地址");
    regEmailLayout->addWidget(regEmailLabel);
    regEmailLayout->addWidget(registerEmail);
    registerLayout->addLayout(regEmailLayout);

    // 注册按钮
    QPushButton *btnRegister = new QPushButton("注册", this);
    btnRegister->setObjectName("btnRegister");
    btnRegister->setMinimumHeight(35);
    btnRegister->setStyleSheet("QPushButton { font-size: 14px; font-weight: bold; }");
    registerLayout->addWidget(btnRegister);

    // 添加底部间距
    registerLayout->addStretch(1);

    tabAuth->addTab(registerTab, "注册");

    leftLayout->addWidget(tabAuth);

    // 添加中间弹性空间（未登录时让登录界面居中）
    leftLayout->addStretch(1);

    // 创建菜单树
    treeWidgetMenu = new QTreeWidget(this);
    treeWidgetMenu->setHeaderHidden(true);
    treeWidgetMenu->setVisible(false);
    treeWidgetMenu->setStyleSheet(
        "QTreeWidget { border: 1px solid #c0c0c0; border-radius: 5px; background-color: #fafafa; }"
        "QTreeWidget::item { padding: 8px; border-bottom: 1px solid #e0e0e0; }"
        "QTreeWidget::item:selected { background-color: #0078d4; color: white; }"
        "QTreeWidget::item:hover { background-color: #e5f3ff; }"
    );
    leftLayout->addWidget(treeWidgetMenu);

    // 添加底部弹性空间
    leftLayout->addStretch(1);

    splitter->addWidget(leftPanel);

    // 创建右侧内容区域
    stackedWidgetContent = new QStackedWidget(this);
    stackedWidgetContent->setStyleSheet("QStackedWidget { background-color: white; }");

    // 欢迎页面
    QWidget *welcomePage = new QWidget();
    welcomePage->setStyleSheet("QWidget { background-color: white; }");
    QVBoxLayout *welcomeLayout = new QVBoxLayout(welcomePage);
    welcomeLayout->setContentsMargins(50, 50, 50, 50);
    welcomeLayout->addStretch();

    QLabel *welcomeLabel = new QLabel("欢迎使用文档管理系统", this);
    welcomeLabel->setObjectName("labelWelcome");
    welcomeLabel->setAlignment(Qt::AlignCenter);
    welcomeLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #333; margin-bottom: 20px;");
    welcomeLayout->addWidget(welcomeLabel);

    QLabel *welcomeDesc = new QLabel("请先登录以使用系统功能", this);
    welcomeDesc->setObjectName("labelWelcomeDesc");
    welcomeDesc->setAlignment(Qt::AlignCenter);
    welcomeDesc->setStyleSheet("font-size: 16px; color: #666; margin-top: 10px;");
    welcomeLayout->addWidget(welcomeDesc);

    welcomeLayout->addStretch();

    stackedWidgetContent->addWidget(welcomePage);
    stackedWidgetContent->setCurrentWidget(welcomePage);

    splitter->addWidget(stackedWidgetContent);

    // 设置分割器比例
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    // 连接信号
    connect(btnLogin, &QPushButton::clicked, this, &MainWindow::on_btnLogin_clicked);
    connect(btnRegister, &QPushButton::clicked, this, &MainWindow::on_btnRegisterUser_clicked);
    connect(btnLogout, &QPushButton::clicked, this, &MainWindow::on_btnLogout_clicked);
    connect(btnQuit, &QPushButton::clicked, this, &MainWindow::on_btnQuit_clicked);
    connect(btnChangePassword, &QPushButton::clicked, this, &MainWindow::on_btnChangePasswordDialog_clicked);
    connect(treeWidgetMenu, &QTreeWidget::itemClicked, this, &MainWindow::onMenuItemClicked);
}

void MainWindow::setupMenuTree()
{
    // 创建主要功能菜单项
    QTreeWidgetItem *documentItem = new QTreeWidgetItem(treeWidgetMenu);
    documentItem->setText(0, "文档管理");
    documentItem->setData(0, Qt::UserRole, "document_root");
    documentItem->setExpanded(true);

    QTreeWidgetItem *myDocsItem = new QTreeWidgetItem(documentItem);
    myDocsItem->setText(0, "我的文档");
    myDocsItem->setData(0, Qt::UserRole, "my_documents");

    QTreeWidgetItem *sharedDocsItem = new QTreeWidgetItem(documentItem);
    sharedDocsItem->setText(0, "分享给我的文档");
    sharedDocsItem->setData(0, Qt::UserRole, "shared_documents");

    QTreeWidgetItem *userItem = new QTreeWidgetItem(treeWidgetMenu);
    userItem->setText(0, "用户管理");
    userItem->setData(0, Qt::UserRole, "user_management");
    userItem->setExpanded(true);
}

void MainWindow::setupContentPages()
{
    // 创建用户管理页面
    pageUserManagement = new QWidget();
    QVBoxLayout *userLayout = new QVBoxLayout(pageUserManagement);

    // 用户搜索区域
    QHBoxLayout *searchLayout = new QHBoxLayout();
    QLineEdit *searchUser = new QLineEdit();
    searchUser->setObjectName("lineEditSearchUser");
    searchUser->setPlaceholderText("输入用户名或邮箱关键词");
    QPushButton *btnSearchUser = new QPushButton("搜索");
    btnSearchUser->setObjectName("btnSearchUser");

    searchLayout->addWidget(searchUser);
    searchLayout->addWidget(btnSearchUser);
    userLayout->addLayout(searchLayout);

    // 用户操作按钮
    QHBoxLayout *userActionsLayout = new QHBoxLayout();
    QPushButton *btnExportUsers = new QPushButton("导出用户");
    btnExportUsers->setObjectName("btnExportUsers");
    QPushButton *btnDownloadTemplate = new QPushButton("下载导入模板");
    btnDownloadTemplate->setObjectName("btnDownloadUserTemplate");
    QPushButton *btnImportUsers = new QPushButton("导入用户");
    btnImportUsers->setObjectName("btnImportUsers");

    userActionsLayout->addWidget(btnExportUsers);
    userActionsLayout->addWidget(btnDownloadTemplate);
    userActionsLayout->addWidget(btnImportUsers);
    userActionsLayout->addStretch();
    userLayout->addLayout(userActionsLayout);

    // 用户表格
    QTableWidget *tableUsers = new QTableWidget();
    tableUsers->setObjectName("tableWidgetUsers");
    userLayout->addWidget(tableUsers);

    stackedWidgetContent->addWidget(pageUserManagement);

    // 创建文档管理页面
    pageDocumentManagement = new QWidget();
    QVBoxLayout *docLayout = new QVBoxLayout(pageDocumentManagement);

    QLabel *docLabel = new QLabel("我的文档", this);
    docLabel->setAlignment(Qt::AlignCenter);
    docLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin: 20px;");
    docLayout->addWidget(docLabel);

    QPushButton *btnViewDocs = new QPushButton("查看我的文档", this);
    btnViewDocs->setObjectName("btnViewDocs");
    docLayout->addWidget(btnViewDocs);

    docLayout->addStretch();
    stackedWidgetContent->addWidget(pageDocumentManagement);

    // 创建分享文档页面
    pageSharedDocuments = new QWidget();
    QVBoxLayout *sharedLayout = new QVBoxLayout(pageSharedDocuments);

    QLabel *sharedLabel = new QLabel("分享给我的文档", this);
    sharedLabel->setAlignment(Qt::AlignCenter);
    sharedLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin: 20px;");
    sharedLayout->addWidget(sharedLabel);

    QPushButton *btnViewSharedDocs = new QPushButton("查看分享文档", this);
    btnViewSharedDocs->setObjectName("btnViewSharedDocs");
    sharedLayout->addWidget(btnViewSharedDocs);

    sharedLayout->addStretch();
    stackedWidgetContent->addWidget(pageSharedDocuments);

    // 连接信号
    connect(btnSearchUser, &QPushButton::clicked, this, &MainWindow::on_btnSearchUser_clicked);
    connect(btnExportUsers, &QPushButton::clicked, this, &MainWindow::on_btnExportUsers_clicked);
    connect(btnDownloadTemplate, &QPushButton::clicked, this, &MainWindow::on_btnDownloadUserTemplate_clicked);
    connect(btnImportUsers, &QPushButton::clicked, this, &MainWindow::on_btnImportUsers_clicked);
    connect(btnViewDocs, &QPushButton::clicked, this, &MainWindow::on_btnViewDocs_clicked);
    connect(btnViewSharedDocs, &QPushButton::clicked, this, &MainWindow::on_btnViewSharedDocs_clicked);
}

void MainWindow::onMenuItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    if (!item) return;

    QString itemData = item->data(0, Qt::UserRole).toString();

    if (itemData == "user_management") {
        showUserManagementPage();
    } else if (itemData == "my_documents") {
        showDocumentManagementPage();
    } else if (itemData == "shared_documents") {
        showSharedDocumentsPage();
    }
}

void MainWindow::showUserManagementPage()
{
    stackedWidgetContent->setCurrentWidget(pageUserManagement);
    updateUserList();
}

void MainWindow::showDocumentManagementPage()
{
    stackedWidgetContent->setCurrentWidget(pageDocumentManagement);
}

void MainWindow::showSharedDocumentsPage()
{
    stackedWidgetContent->setCurrentWidget(pageSharedDocuments);
}



void MainWindow::updateMenuPermissions()
{
    if (!treeWidgetMenu) return;

    // 获取当前用户信息
    auto currentUserResult = g_cliHandler ? g_cliHandler->getCurrentUserForUI() : std::make_pair(false, User{});
    bool isLoggedIn = currentUserResult.first;

    // 遍历所有菜单项并设置权限
    for (int i = 0; i < treeWidgetMenu->topLevelItemCount(); ++i) {
        QTreeWidgetItem *topItem = treeWidgetMenu->topLevelItem(i);
        QString itemData = topItem->data(0, Qt::UserRole).toString();

        if (itemData == "document_root") {
            // 文档管理功能：需要登录
            topItem->setHidden(!isLoggedIn);

            // 设置子项权限
            for (int j = 0; j < topItem->childCount(); ++j) {
                QTreeWidgetItem *childItem = topItem->child(j);
                QString childData = childItem->data(0, Qt::UserRole).toString();

                if (childData == "my_documents" || childData == "shared_documents") {
                    childItem->setHidden(!isLoggedIn);
                }
            }
        } else if (itemData == "user_management") {
            // 用户管理功能：需要登录（后续可以添加管理员权限检查）
            topItem->setHidden(!isLoggedIn);
        }
    }
}

bool MainWindow::hasPermission(const QString& permission)
{
    // 获取当前用户信息
    auto currentUserResult = g_cliHandler ? g_cliHandler->getCurrentUserForUI() : std::make_pair(false, User{});
    if (!currentUserResult.first) {
        return false; // 未登录用户没有任何权限
    }

    User currentUser = currentUserResult.second;

    // 基础权限检查
    if (permission == "login_required") {
        return true; // 已登录用户都有此权限
    }

    // 文档相关权限
    if (permission == "view_documents" || permission == "manage_documents") {
        return true; // 所有登录用户都可以管理自己的文档
    }

    // 用户管理权限（目前所有登录用户都可以查看用户列表，后续可以改为仅管理员）
    if (permission == "view_users") {
        return true;
    }

    // 管理员权限（后续可以根据用户角色字段判断）
    if (permission == "admin") {
        // TODO: 检查用户是否为管理员
        // return currentUser.role == "admin";
        return true; // 暂时所有用户都有管理员权限
    }

    return false;
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


