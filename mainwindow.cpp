#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include "CLIHandler.h"
#include "DatabaseManager.h" // 为了 User 结构体
#include "Common.h" // 为了 Document 结构体
#include <QPushButton>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QDialog>
#include <QLabel>
#include <QTextEdit>
#include <QFileInfo>
#include <QComboBox>
#include "AuthManager.h"
#include "DocListDialog.h"
#include "ChangePasswordDialog.h"
#include "ShareDocumentDialog.h"
#include "PermissionMacros.h"
#include "PermissionDialog.h"
#include <QFileDialog>
#include <QDebug>
#include <QKeyEvent>
#include <QShortcut>
#include <QDateTime>
#include <QInputDialog>

extern CLIHandler* g_cliHandler; // 假设有全局CLIHandler指针

// 权限管理全局变量
PermissionManager* g_permissionManager = nullptr;
int g_currentUserId = 0;

// 获取当前用户ID的函数实现
int getCurrentUserId() {
    return g_currentUserId;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 设置窗口最大化到桌面全屏
    showMaximized();

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

    // 初始化权限管理器
    if (g_cliHandler) {
        g_permissionManager = g_cliHandler->getPermissionManager();
    }

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
        auto result = g_cliHandler->loginUser(username.toUtf8().constData(), password.toUtf8().constData());
        if (result.success) {
            // 恢复欢迎页面的原始提示文字
            QLabel *welcomeDesc = findChild<QLabel*>("labelWelcomeDesc");
            if (welcomeDesc) {
                welcomeDesc->setText("请先登录以使用系统功能");
            }
            updateUserList();
            updateCurrentUserInfo();
        }
        else {
            showErrorDialog("登录失败", QString::fromUtf8(result.message));
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
    auto result = g_cliHandler->logoutUser();
    if (result.success) {
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
        showErrorDialog("登出失败", QString::fromUtf8(result.message));
    }
}

void MainWindow::updateUserList()
{
    if (!g_cliHandler) {
        return;
    }

    // 查找用户表格
    QTableWidget *userTable = findChild<QTableWidget*>("tableWidgetUsers");
    if (!userTable) {
        return;
    }

    // 获取用户列表
    std::vector<User> users = g_cliHandler->getAllUsersForUI();

    updateUserTableWithData(userTable, users);
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
    auto result = g_cliHandler->deleteUser(userId);
    if (result.success) {
        QMessageBox::information(this, "删除用户", "删除成功！");
        updateUserList();
        updateCurrentUserInfo();
    } else {
        QMessageBox::warning(this, "删除用户", "删除失败: " + QString::fromUtf8(result.message));
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
    if (isActive) {
        if (QMessageBox::question(this, "确认禁用", QString("确定要禁用用户ID %1 吗？").arg(userId)) != QMessageBox::Yes) {
            return;
        }
        auto result = g_cliHandler->deactivateUser(userId);
        if (result.success) {
            QMessageBox::information(this, "禁用用户", "禁用成功！");
            updateUserList();
            updateCurrentUserInfo();
        } else {
            QMessageBox::warning(this, "禁用用户", "禁用失败: " + QString::fromUtf8(result.message));
        }
    } else {
        if (QMessageBox::question(this, "确认激活", QString("确定要激活用户ID %1 吗？").arg(userId)) != QMessageBox::Yes) {
            return;
        }
        auto result = g_cliHandler->activateUser(userId);
        if (result.success) {
            QMessageBox::information(this, "激活用户", "激活成功！");
            updateUserList();
            updateCurrentUserInfo();
        } else {
            QMessageBox::warning(this, "激活用户", "激活失败: " + QString::fromUtf8(result.message));
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

    auto result = g_cliHandler->registerUser(username.toUtf8().toStdString(), password.toUtf8().toStdString(), email.toUtf8().toStdString());
    if (result.success) {
        QMessageBox::information(this, "注册", "注册成功！");
        usernameEdit->clear();
        passwordEdit->clear();
        emailEdit->clear();
        updateUserList();
        updateCurrentUserInfo();
    } else {
        showErrorDialog("注册失败", QString::fromUtf8(result.message));
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

        // 设置当前用户ID
        g_currentUserId = user.id;

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
        // 清除当前用户ID
        g_currentUserId = 0;

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

        auto result = g_cliHandler->changeUserPassword(oldPass.toUtf8().toStdString(), newPass.toUtf8().toStdString());

        if (result.success) {
            QMessageBox::information(this, "修改密码", "密码修改成功，请重新登录");
            // 自动登出
            g_cliHandler->logoutUser();
            updateUserList();
            updateCurrentUserInfo();
        } else {
            showErrorDialog("密码修改失败", QString::fromUtf8(result.message));
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

    // 创建菜单树（放在最上面）
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

    // 添加权限管理菜单项
    QTreeWidgetItem *permissionItem = new QTreeWidgetItem(treeWidgetMenu);
    permissionItem->setText(0, "权限管理");
    permissionItem->setData(0, Qt::UserRole, "permission_management");
    permissionItem->setExpanded(true);
}

void MainWindow::setupContentPages()
{
    // 创建用户管理页面
    pageUserManagement = new QWidget();
    pageUserManagement->setStyleSheet("QWidget { background-color: white; }");
    QVBoxLayout *userLayout = new QVBoxLayout(pageUserManagement);
    userLayout->setContentsMargins(20, 20, 20, 20);
    userLayout->setSpacing(15);

    // 页面标题
    QLabel *userLabel = new QLabel("用户管理", this);
    userLabel->setAlignment(Qt::AlignLeft);
    userLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #333; margin-bottom: 10px;");
    userLayout->addWidget(userLabel);

    // 工具栏
    QHBoxLayout *userToolbarLayout = new QHBoxLayout();

    // 搜索框
    QLineEdit *searchEditUsers = new QLineEdit(this);
    searchEditUsers->setObjectName("searchEditUsers");
    searchEditUsers->setPlaceholderText("搜索用户名或邮箱...");
    searchEditUsers->setMaximumWidth(300);
    searchEditUsers->setStyleSheet("QLineEdit { border: 1px solid #ccc; padding: 8px 12px; border-radius: 4px; font-size: 14px; } QLineEdit:focus { border-color: #0078d4; }");

    QPushButton *btnSearchUsers = new QPushButton("搜索", this);
    btnSearchUsers->setObjectName("btnSearchUsers");
    btnSearchUsers->setStyleSheet("QPushButton { background-color: #6c757d; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #5a6268; }");
    connect(btnSearchUsers, &QPushButton::clicked, this, &MainWindow::onSearchUsersClicked);

    QPushButton *btnResetUsers = new QPushButton("重置", this);
    btnResetUsers->setObjectName("btnResetUsers");
    btnResetUsers->setStyleSheet("QPushButton { background-color: #17a2b8; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #138496; }");
    connect(btnResetUsers, &QPushButton::clicked, this, &MainWindow::onResetUsersClicked);

    QPushButton *btnAddUser = new QPushButton("添加用户", this);
    btnAddUser->setObjectName("btnAddUser");
    btnAddUser->setStyleSheet("QPushButton { background-color: #0078d4; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #106ebe; }");
    connect(btnAddUser, &QPushButton::clicked, this, &MainWindow::onAddUserClicked);

    QPushButton *btnExportUsers = new QPushButton("导出Excel", this);
    btnExportUsers->setObjectName("btnExportUsers");
    btnExportUsers->setStyleSheet("QPushButton { background-color: #28a745; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #218838; }");
    connect(btnExportUsers, &QPushButton::clicked, this, &MainWindow::onExportUsersClicked);

    userToolbarLayout->addWidget(searchEditUsers);
    userToolbarLayout->addWidget(btnSearchUsers);
    userToolbarLayout->addWidget(btnResetUsers);
    userToolbarLayout->addWidget(btnAddUser);
    userToolbarLayout->addWidget(btnExportUsers);
    userToolbarLayout->addStretch();
    userLayout->addLayout(userToolbarLayout);

    // 用户列表表格
    QTableWidget *userTableWidget = new QTableWidget(this);
    userTableWidget->setObjectName("tableWidgetUsers");
    userTableWidget->setStyleSheet(
        "QTableWidget { "
            "border: 1px solid #d0d0d0; "
            "border-radius: 5px; "
            "background-color: white; "
            "gridline-color: #e0e0e0; "
            "selection-background-color: #e5f3ff; "
        "}"
        "QTableWidget::item { "
            "padding: 8px; "
            "border-bottom: 1px solid #f0f0f0; "
            "min-height: 35px; "
        "}"
        "QTableWidget::item:selected { "
            "background-color: #e5f3ff; "
        "}"
        "QHeaderView::section { "
            "background-color: #f8f9fa; "
            "border: none; "
            "border-bottom: 2px solid #28a745; "
            "padding: 10px; "
            "font-weight: bold; "
            "color: #333; "
        "}"
    );
    // 设置默认行高
    userTableWidget->verticalHeader()->setDefaultSectionSize(40);
    userTableWidget->verticalHeader()->setVisible(false); // 隐藏行号
    userLayout->addWidget(userTableWidget);

    stackedWidgetContent->addWidget(pageUserManagement);

    // 创建文档管理页面
    pageDocumentManagement = new QWidget();
    pageDocumentManagement->setStyleSheet("QWidget { background-color: white; }");
    QVBoxLayout *docLayout = new QVBoxLayout(pageDocumentManagement);
    docLayout->setContentsMargins(20, 20, 20, 20);
    docLayout->setSpacing(15);

    // 页面标题
    QLabel *docLabel = new QLabel("我的文档", this);
    docLabel->setAlignment(Qt::AlignLeft);
    docLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #333; margin-bottom: 10px;");
    docLayout->addWidget(docLabel);

    // 工具栏
    QHBoxLayout *toolbarLayout = new QHBoxLayout();

    // 搜索框
    QLineEdit *searchEdit = new QLineEdit(this);
    searchEdit->setObjectName("searchEditDocuments");
    searchEdit->setPlaceholderText("搜索文档标题或描述...");
    searchEdit->setMaximumWidth(300);
    searchEdit->setStyleSheet("QLineEdit { border: 1px solid #ccc; padding: 8px 12px; border-radius: 4px; font-size: 14px; } QLineEdit:focus { border-color: #0078d4; }");

    QPushButton *btnSearchDocs = new QPushButton("搜索", this);
    btnSearchDocs->setObjectName("btnSearchDocuments");
    btnSearchDocs->setStyleSheet("QPushButton { background-color: #6c757d; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #5a6268; }");
    connect(btnSearchDocs, &QPushButton::clicked, this, &MainWindow::onSearchDocumentsClicked);

    QPushButton *btnResetDocs = new QPushButton("重置", this);
    btnResetDocs->setObjectName("btnResetDocuments");
    btnResetDocs->setStyleSheet("QPushButton { background-color: #17a2b8; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #138496; }");
    connect(btnResetDocs, &QPushButton::clicked, this, &MainWindow::onResetDocumentsClicked);

    QPushButton *btnUploadDocPage = new QPushButton("上传文档", this);
    btnUploadDocPage->setObjectName("btnUploadDocument");
    btnUploadDocPage->setStyleSheet("QPushButton { background-color: #0078d4; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #106ebe; }");
    connect(btnUploadDocPage, &QPushButton::clicked, this, &MainWindow::onUploadDocumentClicked);

    QPushButton *btnExportDocs = new QPushButton("导出Excel", this);
    btnExportDocs->setObjectName("btnExportDocuments");
    btnExportDocs->setStyleSheet("QPushButton { background-color: #28a745; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #218838; }");
    connect(btnExportDocs, &QPushButton::clicked, this, &MainWindow::onExportDocumentsClicked);

    toolbarLayout->addWidget(searchEdit);
    toolbarLayout->addWidget(btnSearchDocs);
    toolbarLayout->addWidget(btnResetDocs);
    toolbarLayout->addWidget(btnUploadDocPage);
    toolbarLayout->addWidget(btnExportDocs);
    toolbarLayout->addStretch();
    docLayout->addLayout(toolbarLayout);

    // 文档列表表格
    QTableWidget *docTableWidget = new QTableWidget(this);
    docTableWidget->setObjectName("tableWidgetDocuments");
    docTableWidget->setStyleSheet(
        "QTableWidget { "
            "border: 1px solid #d0d0d0; "
            "border-radius: 5px; "
            "background-color: white; "
            "gridline-color: #e0e0e0; "
            "selection-background-color: #e5f3ff; "
        "}"
        "QTableWidget::item { "
            "padding: 8px; "
            "border-bottom: 1px solid #f0f0f0; "
            "min-height: 35px; "
        "}"
        "QTableWidget::item:selected { "
            "background-color: #e5f3ff; "
        "}"
        "QHeaderView::section { "
            "background-color: #f8f9fa; "
            "border: none; "
            "border-bottom: 2px solid #0078d4; "
            "padding: 10px; "
            "font-weight: bold; "
            "color: #333; "
        "}"
    );
    // 设置默认行高
    docTableWidget->verticalHeader()->setDefaultSectionSize(40);
    docTableWidget->verticalHeader()->setVisible(false); // 隐藏行号
    docLayout->addWidget(docTableWidget);

    stackedWidgetContent->addWidget(pageDocumentManagement);

    // 创建分享文档页面
    pageSharedDocuments = new QWidget();
    pageSharedDocuments->setStyleSheet("QWidget { background-color: white; }");
    QVBoxLayout *sharedLayout = new QVBoxLayout(pageSharedDocuments);
    sharedLayout->setContentsMargins(20, 20, 20, 20);
    sharedLayout->setSpacing(15);

    // 页面标题
    QLabel *sharedLabel = new QLabel("分享给我的文档", this);
    sharedLabel->setAlignment(Qt::AlignLeft);
    sharedLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #333; margin-bottom: 10px;");
    sharedLayout->addWidget(sharedLabel);

    // 工具栏
    QHBoxLayout *sharedToolbarLayout = new QHBoxLayout();

    // 搜索框
    QLineEdit *searchEditShared = new QLineEdit(this);
    searchEditShared->setObjectName("searchEditSharedDocuments");
    searchEditShared->setPlaceholderText("搜索分享文档...");
    searchEditShared->setMaximumWidth(300);
    searchEditShared->setStyleSheet("QLineEdit { border: 1px solid #ccc; padding: 8px 12px; border-radius: 4px; font-size: 14px; } QLineEdit:focus { border-color: #17a2b8; }");

    QPushButton *btnSearchShared = new QPushButton("搜索", this);
    btnSearchShared->setObjectName("btnSearchSharedDocuments");
    btnSearchShared->setStyleSheet("QPushButton { background-color: #6c757d; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #5a6268; }");
    connect(btnSearchShared, &QPushButton::clicked, this, &MainWindow::onSearchSharedDocumentsClicked);

    QPushButton *btnResetShared = new QPushButton("重置", this);
    btnResetShared->setObjectName("btnResetSharedDocuments");
    btnResetShared->setStyleSheet("QPushButton { background-color: #17a2b8; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #138496; }");
    connect(btnResetShared, &QPushButton::clicked, this, &MainWindow::onResetSharedDocumentsClicked);

    sharedToolbarLayout->addWidget(searchEditShared);
    sharedToolbarLayout->addWidget(btnSearchShared);
    sharedToolbarLayout->addWidget(btnResetShared);
    sharedToolbarLayout->addStretch();
    sharedLayout->addLayout(sharedToolbarLayout);

    // 分享文档列表表格
    QTableWidget *sharedDocTableWidget = new QTableWidget(this);
    sharedDocTableWidget->setObjectName("tableWidgetSharedDocuments");
    sharedDocTableWidget->setStyleSheet(
        "QTableWidget { "
            "border: 1px solid #d0d0d0; "
            "border-radius: 5px; "
            "background-color: white; "
            "gridline-color: #e0e0e0; "
            "selection-background-color: #e5f3ff; "
        "}"
        "QTableWidget::item { "
            "padding: 8px; "
            "border-bottom: 1px solid #f0f0f0; "
            "min-height: 35px; "
        "}"
        "QTableWidget::item:selected { "
            "background-color: #e5f3ff; "
        "}"
        "QHeaderView::section { "
            "background-color: #f8f9fa; "
            "border: none; "
            "border-bottom: 2px solid #17a2b8; "
            "padding: 10px; "
            "font-weight: bold; "
            "color: #333; "
        "}"
    );
    // 设置默认行高
    sharedDocTableWidget->verticalHeader()->setDefaultSectionSize(60);
    sharedDocTableWidget->verticalHeader()->setVisible(false); // 隐藏行号
    sharedLayout->addWidget(sharedDocTableWidget);

    stackedWidgetContent->addWidget(pageSharedDocuments);

    // 按钮信号已在创建时连接
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
    } else if (itemData == "permission_management") {
        showPermissionManagementDialog();
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
    updateDocumentList();
}

void MainWindow::showSharedDocumentsPage()
{
    stackedWidgetContent->setCurrentWidget(pageSharedDocuments);
    updateSharedDocumentList();
}

void MainWindow::updateDocumentList()
{
    if (!g_cliHandler) {
        return;
    }

    // 获取当前用户
    auto currentUserResult = g_cliHandler->getCurrentUserForUI();
    if (!currentUserResult.first) {
        return;
    }

    User currentUser = currentUserResult.second;

    // 查找文档表格
    QTableWidget *docTable = findChild<QTableWidget*>("tableWidgetDocuments");
    if (!docTable) {
        return;
    }

    // 获取用户文档列表
    std::vector<Document> docs = g_cliHandler->getUserDocsForUI(currentUser.id);

    updateDocumentTableWithData(docTable, docs);
}

void MainWindow::updateSharedDocumentList()
{
    if (!g_cliHandler) {
        return;
    }

    // 获取当前用户
    auto currentUserResult = g_cliHandler->getCurrentUserForUI();
    if (!currentUserResult.first) {
        return;
    }

    User currentUser = currentUserResult.second;

    // 查找分享文档表格
    QTableWidget *sharedDocTable = findChild<QTableWidget*>("tableWidgetSharedDocuments");
    if (!sharedDocTable) {
        return;
    }

    // 获取分享给当前用户的文档列表
    std::vector<Document> docs = g_cliHandler->getSharedDocsForUI(currentUser.id);

    updateSharedDocumentTableWithData(sharedDocTable, docs);
}

void MainWindow::onUploadDocumentClicked()
{
    // 创建上传文档对话框
    QDialog uploadDialog(this);
    uploadDialog.setWindowTitle("上传文档");
    uploadDialog.setModal(true);
    uploadDialog.resize(400, 300);

    QVBoxLayout *layout = new QVBoxLayout(&uploadDialog);

    // 文件选择
    QHBoxLayout *fileLayout = new QHBoxLayout();
    QLabel *fileLabel = new QLabel("选择文件:");
    QLineEdit *fileEdit = new QLineEdit();
    fileEdit->setReadOnly(true);
    QPushButton *browseBtn = new QPushButton("浏览...");

    fileLayout->addWidget(fileLabel);
    fileLayout->addWidget(fileEdit);
    fileLayout->addWidget(browseBtn);
    layout->addLayout(fileLayout);

    // 文档标题
    QHBoxLayout *titleLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("文档标题:");
    QLineEdit *titleEdit = new QLineEdit();
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(titleEdit);
    layout->addLayout(titleLayout);

    // 文档描述
    QVBoxLayout *descLayout = new QVBoxLayout();
    QLabel *descLabel = new QLabel("文档描述:");
    QTextEdit *descEdit = new QTextEdit();
    descEdit->setMaximumHeight(100);
    descLayout->addWidget(descLabel);
    descLayout->addWidget(descEdit);
    layout->addLayout(descLayout);

    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okBtn = new QPushButton("上传");
    QPushButton *cancelBtn = new QPushButton("取消");
    buttonLayout->addStretch();
    buttonLayout->addWidget(okBtn);
    buttonLayout->addWidget(cancelBtn);
    layout->addLayout(buttonLayout);

    // 连接信号
    connect(browseBtn, &QPushButton::clicked, [&]() {
        QString fileName = QFileDialog::getOpenFileName(&uploadDialog, "选择要上传的文档", "", "所有文件 (*.*)");
        if (!fileName.isEmpty()) {
            fileEdit->setText(fileName);
            // 自动填充标题（使用文件名）
            QFileInfo fileInfo(fileName);
            if (titleEdit->text().isEmpty()) {
                titleEdit->setText(fileInfo.baseName());
            }
        }
    });

    connect(okBtn, &QPushButton::clicked, [&]() {
        if (fileEdit->text().isEmpty()) {
            QMessageBox::warning(&uploadDialog, "错误", "请选择要上传的文件");
            return;
        }
        if (titleEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(&uploadDialog, "错误", "请输入文档标题");
            return;
        }

        auto result = g_cliHandler->handleAddDocument(
            titleEdit->text().toUtf8().toStdString(),
            descEdit->toPlainText().toUtf8().toStdString(),
            fileEdit->text().toUtf8().toStdString()
        );
        if (result) {
            QMessageBox::information(&uploadDialog, "上传成功", "文档已成功上传");
            uploadDialog.accept();
            updateDocumentList(); // 刷新列表
        } else {
            QMessageBox::warning(&uploadDialog, "上传失败", "上传文档时出现错误: ");
        }
    });

    connect(cancelBtn, &QPushButton::clicked, [&]() {
        uploadDialog.reject();
    });

    uploadDialog.exec();
}

void MainWindow::onSearchDocumentsClicked()
{
    QLineEdit *searchEdit = findChild<QLineEdit*>("searchEditDocuments");
    if (!searchEdit) return;

    QString keyword = searchEdit->text().trimmed();
    if (keyword.isEmpty()) {
        updateDocumentList(); // 如果搜索框为空，显示所有文档
        return;
    }

    if (!g_cliHandler) return;

    // 获取当前用户
    auto currentUserResult = g_cliHandler->getCurrentUserForUI();
    if (!currentUserResult.first) return;

    User currentUser = currentUserResult.second;

    // 搜索文档
    std::vector<Document> docs = g_cliHandler->getSearchedDocsForUI(currentUser.id, keyword.toUtf8().toStdString());

    // 更新表格显示
    QTableWidget *docTable = findChild<QTableWidget*>("tableWidgetDocuments");
    if (!docTable) return;

    updateDocumentTableWithData(docTable, docs);
}

void MainWindow::onResetDocumentsClicked()
{
    // 清空搜索框
    QLineEdit *searchEdit = findChild<QLineEdit*>("searchEditDocuments");
    if (searchEdit) {
        searchEdit->clear();
    }

    // 重新加载完整的文档列表
    updateDocumentList();
}

void MainWindow::onExportDocumentsClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出文档列表", "documents_export.xlsx", "Excel文件 (*.xlsx)");
    if (!fileName.isEmpty()) {
        if (g_cliHandler) {
            // 获取当前用户
            auto currentUserResult = g_cliHandler->getCurrentUserForUI();
            if (currentUserResult.first) {
                User currentUser = currentUserResult.second;
                auto result = g_cliHandler->exportDocumentsToExcel(fileName.toUtf8().toStdString(), true);
                if (result.success) {
                    QMessageBox::information(this, "导出成功", "文档列表已导出到: " + fileName);
                } else {
                    QMessageBox::warning(this, "导出失败", "导出文档列表时出现错误: " + QString::fromUtf8(result.message));
                }
            }
        }
    }
}

void MainWindow::onSearchSharedDocumentsClicked()
{
    QLineEdit *searchEdit = findChild<QLineEdit*>("searchEditSharedDocuments");
    if (!searchEdit) return;

    QString keyword = searchEdit->text().trimmed();
    if (keyword.isEmpty()) {
        updateSharedDocumentList(); // 如果搜索框为空，显示所有分享文档
        return;
    }

    if (!g_cliHandler) return;

    // 获取当前用户
    auto currentUserResult = g_cliHandler->getCurrentUserForUI();
    if (!currentUserResult.first) return;

    User currentUser = currentUserResult.second;

    // 获取分享文档并进行本地过滤
    std::vector<Document> allDocs = g_cliHandler->getSharedDocsForUI(currentUser.id);
    std::vector<Document> filteredDocs;

    QString lowerKeyword = keyword.toLower();
    for (const auto& doc : allDocs) {
        QString title = QString::fromUtf8(doc.title).toLower();
        QString desc = QString::fromUtf8(doc.description).toLower();
        if (title.contains(lowerKeyword) || desc.contains(lowerKeyword)) {
            filteredDocs.push_back(doc);
        }
    }

    // 更新表格显示
    QTableWidget *sharedDocTable = findChild<QTableWidget*>("tableWidgetSharedDocuments");
    if (!sharedDocTable) return;

    updateSharedDocumentTableWithData(sharedDocTable, filteredDocs);
}

void MainWindow::onResetSharedDocumentsClicked()
{
    // 清空搜索框
    QLineEdit *searchEdit = findChild<QLineEdit*>("searchEditSharedDocuments");
    if (searchEdit) {
        searchEdit->clear();
    }

    // 重新加载完整的分享文档列表
    updateSharedDocumentList();
}

void MainWindow::updateDocumentTableWithData(QTableWidget *table, const std::vector<Document>& docs)
{
    if (!table) return;

    // 设置表头
    table->clear();
    table->setColumnCount(7);
    QStringList headers;
    headers << "ID" << "标题" << "描述" << "文件名" << "大小" << "创建时间" << "操作";
    table->setHorizontalHeaderLabels(headers);

    // 填充数据
    table->setRowCount(docs.size());
    for (int i = 0; i < docs.size(); ++i) {
        const Document& doc = docs[i];

        table->setItem(i, 0, new QTableWidgetItem(QString::number(doc.id)));
        table->setItem(i, 1, new QTableWidgetItem(QString::fromUtf8(doc.title)));
        table->setItem(i, 2, new QTableWidgetItem(QString::fromUtf8(doc.description)));
        table->setItem(i, 3, new QTableWidgetItem(QString::fromUtf8(doc.file_path)));

        // 格式化文件大小
        QString sizeStr;
        if (doc.file_size < 1024) {
            sizeStr = QString::number(doc.file_size) + " B";
        } else if (doc.file_size < 1024 * 1024) {
            sizeStr = QString::number(doc.file_size / 1024.0, 'f', 1) + " KB";
        } else {
            sizeStr = QString::number(doc.file_size / (1024.0 * 1024.0), 'f', 1) + " MB";
        }
        table->setItem(i, 4, new QTableWidgetItem(sizeStr));

        // 格式化创建时间
        auto time_t = std::chrono::system_clock::to_time_t(doc.created_at);
        QString timeStr = QDateTime::fromSecsSinceEpoch(time_t).toString("yyyy-MM-dd hh:mm");
        table->setItem(i, 5, new QTableWidgetItem(timeStr));

        // 操作按钮
        QWidget *operationWidget = new QWidget();
        QHBoxLayout *operationLayout = new QHBoxLayout(operationWidget);
        operationLayout->setContentsMargins(3, 3, 3, 3);
        operationLayout->setSpacing(2);

        QPushButton *btnDownload = new QPushButton("下载");
        btnDownload->setMinimumWidth(36);
        btnDownload->setMaximumWidth(36);
        btnDownload->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        btnDownload->setProperty("docId", doc.id);
        btnDownload->setStyleSheet("QPushButton { background-color: #28a745; color: white; border: none; padding: 1px; border-radius: 2px; font-size: 10px; font-weight: bold; } QPushButton:hover { background-color: #218838; }");

        QPushButton *btnEdit = new QPushButton("编辑");
        btnEdit->setMinimumWidth(36);
        btnEdit->setMaximumWidth(36);
        btnEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        btnEdit->setProperty("docId", doc.id);
        btnEdit->setStyleSheet("QPushButton { background-color: #ffc107; color: #333; border: none; padding: 1px; border-radius: 2px; font-size: 10px; font-weight: bold; } QPushButton:hover { background-color: #e0a800; }");

        QPushButton *btnDelete = new QPushButton("删除");
        btnDelete->setMinimumWidth(36);
        btnDelete->setMaximumWidth(36);
        btnDelete->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        btnDelete->setProperty("docId", doc.id);
        btnDelete->setStyleSheet("QPushButton { background-color: #dc3545; color: white; border: none; padding: 1px; border-radius: 2px; font-size: 10px; font-weight: bold; } QPushButton:hover { background-color: #c82333; }");

        QPushButton *btnShare = new QPushButton("分享");
        btnShare->setMinimumWidth(36);
        btnShare->setMaximumWidth(36);
        btnShare->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        btnShare->setProperty("docId", doc.id);
        btnShare->setStyleSheet("QPushButton { background-color: #17a2b8; color: white; border: none; padding: 1px; border-radius: 2px; font-size: 10px; font-weight: bold; } QPushButton:hover { background-color: #138496; }");

        // 连接信号
        connect(btnDownload, &QPushButton::clicked, [this, doc]() {
            QString defaultFileName = QString::fromUtf8(doc.file_path);
            if (defaultFileName.isEmpty()) {
                defaultFileName = QString::fromUtf8(doc.title);
            }
            QString fileName = QFileDialog::getSaveFileName(this, "保存文档", defaultFileName);
            if (!fileName.isEmpty()) {
                if (g_cliHandler && g_cliHandler->getMinioClient() && g_cliHandler->getMinioClient()->isInitialized()) {
                    auto result = g_cliHandler->getMinioClient()->getObject(doc.minio_key, fileName.toUtf8().toStdString());
                    if (result.success) {
                        QMessageBox::information(this, "下载成功", "文档已保存到: " + fileName);
                    } else {
                        QMessageBox::warning(this, "下载失败", "下载文档时出现错误: " + QString::fromUtf8(result.message));
                    }
                } else {
                    QMessageBox::warning(this, "下载失败", "MinIO客户端未初始化");
                }
            }
        });

        connect(btnEdit, &QPushButton::clicked, [this, doc]() {
            // 创建编辑文档对话框
            QDialog editDialog(this);
            editDialog.setWindowTitle("编辑文档");
            editDialog.setModal(true);
            editDialog.resize(450, 350);

            QVBoxLayout *layout = new QVBoxLayout(&editDialog);

            // 当前文件信息
            QHBoxLayout *currentFileLayout = new QHBoxLayout();
            QLabel *currentFileLabel = new QLabel("当前文件:");
            QLineEdit *currentFileEdit = new QLineEdit(QString::fromUtf8(doc.file_path));
            currentFileEdit->setReadOnly(true);
            currentFileEdit->setStyleSheet("QLineEdit { background-color: #f0f0f0; }");
            currentFileLayout->addWidget(currentFileLabel);
            currentFileLayout->addWidget(currentFileEdit);
            layout->addLayout(currentFileLayout);

            // 新文件选择（可选）
            QHBoxLayout *newFileLayout = new QHBoxLayout();
            QLabel *newFileLabel = new QLabel("替换文件:");
            QLineEdit *newFileEdit = new QLineEdit();
            newFileEdit->setPlaceholderText("选择新文件替换当前文件（可选）");
            QPushButton *browseBtn = new QPushButton("浏览...");
            newFileLayout->addWidget(newFileLabel);
            newFileLayout->addWidget(newFileEdit);
            newFileLayout->addWidget(browseBtn);
            layout->addLayout(newFileLayout);

            // 文档标题
            QHBoxLayout *titleLayout = new QHBoxLayout();
            QLabel *titleLabel = new QLabel("文档标题:");
            QLineEdit *titleEdit = new QLineEdit(QString::fromUtf8(doc.title));
            titleLayout->addWidget(titleLabel);
            titleLayout->addWidget(titleEdit);
            layout->addLayout(titleLayout);

            // 文档描述
            QVBoxLayout *descLayout = new QVBoxLayout();
            QLabel *descLabel = new QLabel("文档描述:");
            QTextEdit *descEdit = new QTextEdit();
            descEdit->setPlainText(QString::fromUtf8(doc.description));
            descEdit->setMaximumHeight(100);
            descLayout->addWidget(descLabel);
            descLayout->addWidget(descEdit);
            layout->addLayout(descLayout);

            // 按钮
            QHBoxLayout *buttonLayout = new QHBoxLayout();
            QPushButton *okBtn = new QPushButton("保存");
            QPushButton *cancelBtn = new QPushButton("取消");
            buttonLayout->addStretch();
            buttonLayout->addWidget(okBtn);
            buttonLayout->addWidget(cancelBtn);
            layout->addLayout(buttonLayout);

            // 连接信号
            connect(browseBtn, &QPushButton::clicked, [&]() {
                QString fileName = QFileDialog::getOpenFileName(&editDialog, "选择新文件", "", "所有文件 (*.*)");
                if (!fileName.isEmpty()) {
                    newFileEdit->setText(fileName);
                }
            });

            connect(okBtn, &QPushButton::clicked, [&]() {
                if (titleEdit->text().trimmed().isEmpty()) {
                    QMessageBox::warning(&editDialog, "错误", "请输入文档标题");
                    return;
                }

                std::string newFilePath = newFileEdit->text().trimmed().isEmpty() ? "" : newFileEdit->text().toUtf8().toStdString();

                auto result = g_cliHandler->handleUpdateDocument(
                    doc.id,
                    titleEdit->text().toUtf8().toStdString(),
                    descEdit->toPlainText().toUtf8().toStdString(),
                    newFilePath
                );

                if (result) {
                    QString message = newFileEdit->text().trimmed().isEmpty() ?
                        "文档信息已更新" : "文档信息和文件已更新";
                    QMessageBox::information(&editDialog, "编辑成功", message);
                    editDialog.accept();
                    updateDocumentList();
                } else {
                    QMessageBox::warning(&editDialog, "编辑失败", "编辑文档时出现错误: ");
                }
            });

            connect(cancelBtn, &QPushButton::clicked, [&]() {
                editDialog.reject();
            });

            editDialog.exec();
        });

        connect(btnDelete, &QPushButton::clicked, [this, doc]() {
            int ret = QMessageBox::question(this, "确认删除",
                QString("确定要删除文档 \"%1\" 吗？").arg(QString::fromUtf8(doc.title)));
            if (ret == QMessageBox::Yes) {
                auto result = g_cliHandler->handleDeleteDocument(doc.id);
                if (result) {
                    QMessageBox::information(this, "删除成功", "文档已删除");
                    updateDocumentList();
                } else {
                    QMessageBox::warning(this, "删除失败", "删除文档时出现错误");
                }
            }
        });

        connect(btnShare, &QPushButton::clicked, [this, doc]() {
            ShareDocumentDialog shareDialog(doc, this);
            shareDialog.exec();
        });

        operationLayout->addWidget(btnDownload);
        operationLayout->addWidget(btnEdit);
        operationLayout->addWidget(btnDelete);
        operationLayout->addWidget(btnShare);
        operationLayout->addStretch();

        table->setCellWidget(i, 6, operationWidget);
    }

    // 设置列宽
    table->resizeColumnsToContents();
    table->setColumnWidth(6, 190); // 操作列设置合适的固定宽度
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // ID列
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); // 标题列可拉伸
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch); // 描述列可拉伸
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // 文件名列
    table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents); // 大小列
    table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents); // 时间列
    table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed); // 操作列固定
}

void MainWindow::updateSharedDocumentTableWithData(QTableWidget *table, const std::vector<Document>& docs)
{
    if (!table) return;

    // 设置表头
    table->clear();
    table->setColumnCount(7);
    QStringList headers;
    headers << "ID" << "标题" << "描述" << "文件名" << "大小" << "分享时间" << "操作";
    table->setHorizontalHeaderLabels(headers);

    // 填充数据
    table->setRowCount(docs.size());
    for (int i = 0; i < docs.size(); ++i) {
        const Document& doc = docs[i];

        table->setItem(i, 0, new QTableWidgetItem(QString::number(doc.id)));
        table->setItem(i, 1, new QTableWidgetItem(QString::fromUtf8(doc.title)));
        table->setItem(i, 2, new QTableWidgetItem(QString::fromUtf8(doc.description)));
        table->setItem(i, 3, new QTableWidgetItem(QString::fromUtf8(doc.file_path)));

        // 格式化文件大小
        QString sizeStr;
        if (doc.file_size < 1024) {
            sizeStr = QString::number(doc.file_size) + " B";
        } else if (doc.file_size < 1024 * 1024) {
            sizeStr = QString::number(doc.file_size / 1024.0, 'f', 1) + " KB";
        } else {
            sizeStr = QString::number(doc.file_size / (1024.0 * 1024.0), 'f', 1) + " MB";
        }
        table->setItem(i, 4, new QTableWidgetItem(sizeStr));

        // 格式化分享时间
        auto time_t = std::chrono::system_clock::to_time_t(doc.created_at);
        QString timeStr = QDateTime::fromSecsSinceEpoch(time_t).toString("yyyy-MM-dd hh:mm");
        table->setItem(i, 5, new QTableWidgetItem(timeStr));

        // 操作按钮（分享的文档只能下载）
        QWidget *operationWidget = new QWidget();
        QHBoxLayout *operationLayout = new QHBoxLayout(operationWidget);
        operationLayout->setContentsMargins(3, 3, 3, 3);
        operationLayout->setSpacing(2);

        QPushButton *btnDownload = new QPushButton("下载");
        btnDownload->setMinimumWidth(60);
        btnDownload->setMaximumWidth(60);
        btnDownload->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        btnDownload->setProperty("docId", doc.id);
        btnDownload->setStyleSheet("QPushButton { background-color: #28a745; color: white; border: none; padding: 2px; border-radius: 3px; font-size: 11px; font-weight: bold; } QPushButton:hover { background-color: #218838; }");

        // 连接下载信号
        connect(btnDownload, &QPushButton::clicked, [this, doc]() {
            QString defaultFileName = QString::fromUtf8(doc.file_path);
            if (defaultFileName.isEmpty()) {
                defaultFileName = QString::fromUtf8(doc.title);
            }
            QString fileName = QFileDialog::getSaveFileName(this, "保存文档", defaultFileName);
            if (!fileName.isEmpty()) {
                if (g_cliHandler && g_cliHandler->getMinioClient() && g_cliHandler->getMinioClient()->isInitialized()) {
                    auto result = g_cliHandler->getMinioClient()->getObject(doc.minio_key, fileName.toUtf8().toStdString());
                    if (result.success) {
                        QMessageBox::information(this, "下载成功", "文档已保存到: " + fileName);
                    } else {
                        QMessageBox::warning(this, "下载失败", "下载文档时出现错误: " + QString::fromUtf8(result.message));
                    }
                } else {
                    QMessageBox::warning(this, "下载失败", "MinIO客户端未初始化");
                }
            }
        });

        operationLayout->addWidget(btnDownload);
        operationLayout->addStretch();

        table->setCellWidget(i, 6, operationWidget);
    }

    // 设置列宽
    table->resizeColumnsToContents();
    table->setColumnWidth(6, 100); // 操作列设置更宽的固定宽度
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // ID列
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); // 标题列可拉伸
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch); // 描述列可拉伸
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // 文件名列
    table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents); // 大小列
    table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents); // 时间列
    table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed); // 操作列固定
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
    auto result = g_cliHandler->exportUsersToExcel(filePath.toUtf8().toStdString());
    if (result.success) {
        QMessageBox::information(this, "导出用户", "导出成功！");
    } else {
        QMessageBox::warning(this, "导出用户", "导出失败: " + QString::fromUtf8(result.message));
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
    auto result = g_cliHandler->importUsersFromExcel(filePath.toUtf8().toStdString());
    if (result.success) {
        QMessageBox::information(this, "导入成功", "用户数据导入成功！");
        updateUserList();
    } else {
        QMessageBox::warning(this, "导入失败", "用户数据导入失败: " + QString::fromUtf8(result.message));
    }
}

// 新的用户管理方法
void MainWindow::onSearchUsersClicked()
{
    QLineEdit *searchEdit = findChild<QLineEdit*>("searchEditUsers");
    if (!searchEdit) return;

    QString keyword = searchEdit->text().trimmed();
    if (keyword.isEmpty()) {
        updateUserList(); // 如果搜索框为空，显示所有用户
        return;
    }

    if (!g_cliHandler) return;

    // 搜索用户
    std::vector<User> users = g_cliHandler->getSearchedUsersForUI(keyword.toUtf8().toStdString());

    // 更新表格显示
    QTableWidget *userTable = findChild<QTableWidget*>("tableWidgetUsers");
    if (!userTable) return;

    updateUserTableWithData(userTable, users);
}

void MainWindow::onResetUsersClicked()
{
    // 清空搜索框
    QLineEdit *searchEdit = findChild<QLineEdit*>("searchEditUsers");
    if (searchEdit) {
        searchEdit->clear();
    }

    // 重新加载完整的用户列表
    updateUserList();
}

void MainWindow::onAddUserClicked()
{
    // 创建添加用户对话框
    QDialog addDialog(this);
    addDialog.setWindowTitle("添加用户");
    addDialog.setModal(true);
    addDialog.resize(400, 300);

    QVBoxLayout *layout = new QVBoxLayout(&addDialog);

    // 用户名
    QHBoxLayout *usernameLayout = new QHBoxLayout();
    QLabel *usernameLabel = new QLabel("用户名:");
    QLineEdit *usernameEdit = new QLineEdit();
    usernameLayout->addWidget(usernameLabel);
    usernameLayout->addWidget(usernameEdit);
    layout->addLayout(usernameLayout);

    // 密码
    QHBoxLayout *passwordLayout = new QHBoxLayout();
    QLabel *passwordLabel = new QLabel("密码:");
    QLineEdit *passwordEdit = new QLineEdit();
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordLayout->addWidget(passwordLabel);
    passwordLayout->addWidget(passwordEdit);
    layout->addLayout(passwordLayout);

    // 邮箱
    QHBoxLayout *emailLayout = new QHBoxLayout();
    QLabel *emailLabel = new QLabel("邮箱:");
    QLineEdit *emailEdit = new QLineEdit();
    emailLayout->addWidget(emailLabel);
    emailLayout->addWidget(emailEdit);
    layout->addLayout(emailLayout);

    // 状态
    QHBoxLayout *statusLayout = new QHBoxLayout();
    QLabel *statusLabel = new QLabel("状态:");
    QComboBox *statusCombo = new QComboBox();
    statusCombo->addItem("激活", true);
    statusCombo->addItem("禁用", false);
    statusCombo->setCurrentIndex(0); // 默认激活
    statusLayout->addWidget(statusLabel);
    statusLayout->addWidget(statusCombo);
    layout->addLayout(statusLayout);

    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okBtn = new QPushButton("添加");
    QPushButton *cancelBtn = new QPushButton("取消");
    buttonLayout->addStretch();
    buttonLayout->addWidget(okBtn);
    buttonLayout->addWidget(cancelBtn);
    layout->addLayout(buttonLayout);

    connect(okBtn, &QPushButton::clicked, [&]() {
        if (usernameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(&addDialog, "错误", "请输入用户名");
            return;
        }
        if (passwordEdit->text().isEmpty()) {
            QMessageBox::warning(&addDialog, "错误", "请输入密码");
            return;
        }
        if (emailEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(&addDialog, "错误", "请输入邮箱");
            return;
        }

        auto result = g_cliHandler->registerUser(
            usernameEdit->text().toUtf8().toStdString(),
            passwordEdit->text().toUtf8().toStdString(),
            emailEdit->text().toUtf8().toStdString()
        );
        if (result.success) {
            // 如果需要设置为禁用状态
            if (!statusCombo->currentData().toBool()) {
                // 获取刚创建的用户并设置为禁用状态
                auto userResult = g_cliHandler->getDbManager()->getUserByUsername(usernameEdit->text().toUtf8().toStdString());
                if (userResult.success) {
                    User newUser = userResult.data.value();
                    newUser.is_active = false;
                    g_cliHandler->getDbManager()->updateUser(newUser);
                }
            }
            QMessageBox::information(&addDialog, "添加成功", "用户已成功添加");
            addDialog.accept();
            updateUserList();
        } else {
            QMessageBox::warning(&addDialog, "添加失败", "添加用户时出现错误: " + QString::fromUtf8(result.message));
        }
    });

    connect(cancelBtn, &QPushButton::clicked, [&]() {
        addDialog.reject();
    });

    addDialog.exec();
}

void MainWindow::onExportUsersClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出用户列表", "users_export.xlsx", "Excel文件 (*.xlsx)");
    if (!fileName.isEmpty()) {
        if (g_cliHandler) {
            auto result = g_cliHandler->exportUsersToExcel(fileName.toUtf8().toStdString());
            if (result.success) {
                QMessageBox::information(this, "导出成功", "用户列表已导出到: " + fileName);
            } else {
                QMessageBox::warning(this, "导出失败", "导出用户列表时出现错误: " + QString::fromUtf8(result.message));
            }
        }
    }
}

void MainWindow::updateUserTableWithData(QTableWidget *table, const std::vector<User>& users)
{
    if (!table) return;

    // 设置表头
    table->clear();
    table->setColumnCount(6);
    QStringList headers;
    headers << "ID" << "用户名" << "邮箱" << "状态" << "创建时间" << "操作";
    table->setHorizontalHeaderLabels(headers);

    // 填充数据
    table->setRowCount(users.size());
    for (int i = 0; i < users.size(); ++i) {
        const User& user = users[i];

        table->setItem(i, 0, new QTableWidgetItem(QString::number(user.id)));
        table->setItem(i, 1, new QTableWidgetItem(QString::fromUtf8(user.username)));
        table->setItem(i, 2, new QTableWidgetItem(QString::fromUtf8(user.email)));

        // 状态显示
        QString statusStr = user.is_active ? "激活" : "禁用";
        table->setItem(i, 3, new QTableWidgetItem(statusStr));

        // 格式化创建时间
        auto time_t = std::chrono::system_clock::to_time_t(user.created_at);
        QString timeStr = QDateTime::fromSecsSinceEpoch(time_t).toString("yyyy-MM-dd hh:mm");
        table->setItem(i, 4, new QTableWidgetItem(timeStr));

        // 操作按钮
        QWidget *operationWidget = new QWidget();
        QHBoxLayout *operationLayout = new QHBoxLayout(operationWidget);
        operationLayout->setContentsMargins(3, 3, 3, 3);
        operationLayout->setSpacing(2);

        QPushButton *btnEdit = new QPushButton("编辑");
        btnEdit->setMinimumWidth(36);
        btnEdit->setMaximumWidth(36);
        btnEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        btnEdit->setProperty("userId", user.id);
        btnEdit->setStyleSheet("QPushButton { background-color: #ffc107; color: #333; border: none; padding: 1px; border-radius: 2px; font-size: 10px; font-weight: bold; } QPushButton:hover { background-color: #e0a800; }");

        QPushButton *btnDelete = new QPushButton("删除");
        btnDelete->setMinimumWidth(36);
        btnDelete->setMaximumWidth(36);
        btnDelete->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        btnDelete->setProperty("userId", user.id);
        btnDelete->setStyleSheet("QPushButton { background-color: #dc3545; color: white; border: none; padding: 1px; border-radius: 2px; font-size: 10px; font-weight: bold; } QPushButton:hover { background-color: #c82333; }");

        QPushButton *btnResetPwd = new QPushButton("重置");
        btnResetPwd->setMinimumWidth(36);
        btnResetPwd->setMaximumWidth(36);
        btnResetPwd->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        btnResetPwd->setProperty("userId", user.id);
        btnResetPwd->setStyleSheet("QPushButton { background-color: #17a2b8; color: white; border: none; padding: 1px; border-radius: 2px; font-size: 10px; font-weight: bold; } QPushButton:hover { background-color: #138496; }");

        // 连接信号
        connect(btnEdit, &QPushButton::clicked, [this, user]() {
            // 编辑用户对话框
            QDialog editDialog(this);
            editDialog.setWindowTitle("编辑用户");
            editDialog.setModal(true);
            editDialog.resize(400, 300);

            QVBoxLayout *layout = new QVBoxLayout(&editDialog);

            // 用户名
            QHBoxLayout *usernameLayout = new QHBoxLayout();
            QLabel *usernameLabel = new QLabel("用户名:");
            QLineEdit *usernameEdit = new QLineEdit(QString::fromUtf8(user.username));
            usernameLayout->addWidget(usernameLabel);
            usernameLayout->addWidget(usernameEdit);
            layout->addLayout(usernameLayout);

            // 邮箱
            QHBoxLayout *emailLayout = new QHBoxLayout();
            QLabel *emailLabel = new QLabel("邮箱:");
            QLineEdit *emailEdit = new QLineEdit(QString::fromUtf8(user.email));
            emailLayout->addWidget(emailLabel);
            emailLayout->addWidget(emailEdit);
            layout->addLayout(emailLayout);

            // 状态
            QHBoxLayout *statusLayout = new QHBoxLayout();
            QLabel *statusLabel = new QLabel("状态:");
            QComboBox *statusCombo = new QComboBox();
            statusCombo->addItem("激活", true);
            statusCombo->addItem("禁用", false);
            statusCombo->setCurrentText(user.is_active ? "激活" : "禁用");
            statusLayout->addWidget(statusLabel);
            statusLayout->addWidget(statusCombo);
            layout->addLayout(statusLayout);

            // 按钮
            QHBoxLayout *buttonLayout = new QHBoxLayout();
            QPushButton *okBtn = new QPushButton("保存");
            QPushButton *cancelBtn = new QPushButton("取消");
            buttonLayout->addStretch();
            buttonLayout->addWidget(okBtn);
            buttonLayout->addWidget(cancelBtn);
            layout->addLayout(buttonLayout);

            connect(okBtn, &QPushButton::clicked, [&]() {
                if (usernameEdit->text().trimmed().isEmpty()) {
                    QMessageBox::warning(&editDialog, "错误", "请输入用户名");
                    return;
                }
                if (emailEdit->text().trimmed().isEmpty()) {
                    QMessageBox::warning(&editDialog, "错误", "请输入邮箱");
                    return;
                }

                // 更新用户信息
                User updatedUser = user;
                updatedUser.username = usernameEdit->text().toUtf8().toStdString();
                updatedUser.email = emailEdit->text().toUtf8().toStdString();
                updatedUser.is_active = statusCombo->currentData().toBool();

                auto result = g_cliHandler->getDbManager()->updateUser(updatedUser);
                if (result.success) {
                    QMessageBox::information(&editDialog, "编辑成功", "用户信息已更新");
                    editDialog.accept();
                    updateUserList();
                } else {
                    QMessageBox::warning(&editDialog, "编辑失败", "编辑用户时出现错误: " + QString::fromUtf8(result.message));
                }
            });

            connect(cancelBtn, &QPushButton::clicked, [&]() {
                editDialog.reject();
            });

            editDialog.exec();
        });

        connect(btnDelete, &QPushButton::clicked, [this, user]() {
            int ret = QMessageBox::question(this, "确认删除",
                QString("确定要删除用户 \"%1\" 吗？").arg(QString::fromUtf8(user.username)));
            if (ret == QMessageBox::Yes) {
                auto result = g_cliHandler->deleteUser(user.id);
                if (result.success) {
                    QMessageBox::information(this, "删除成功", "用户已删除");
                    updateUserList();
                } else {
                    QMessageBox::warning(this, "删除失败", "删除用户时出现错误: " + QString::fromUtf8(result.message));
                }
            }
        });

        connect(btnResetPwd, &QPushButton::clicked, [this, user]() {
            int ret = QMessageBox::question(this, "确认重置",
                QString("确定要重置用户 \"%1\" 的密码吗？\n密码将重置为: 123456").arg(QString::fromUtf8(user.username)));
            if (ret == QMessageBox::Yes) {
                // 重置密码为默认密码
                User updatedUser = user;
                updatedUser.password_hash = AuthManager::hashPassword("123456");

                auto result = g_cliHandler->getDbManager()->updateUser(updatedUser);
                if (result.success) {
                    QMessageBox::information(this, "重置成功", "密码已重置为: 123456");
                } else {
                    QMessageBox::warning(this, "重置失败", "重置密码时出现错误: " + QString::fromUtf8(result.message));
                }
            }
        });

        operationLayout->addWidget(btnEdit);
        operationLayout->addWidget(btnDelete);
        operationLayout->addWidget(btnResetPwd);
        operationLayout->addStretch();

        table->setCellWidget(i, 5, operationWidget);
    }

    // 设置列宽
    table->resizeColumnsToContents();
    table->setColumnWidth(5, 130); // 操作列设置合适的固定宽度
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // ID列
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); // 用户名列可拉伸
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch); // 邮箱列可拉伸
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // 状态列
    table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents); // 时间列
    table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed); // 操作列固定
}

void MainWindow::showPermissionManagementDialog()
{
    // 检查权限
    if (!HAS_PERMISSION("permission:assign")) {
        QMessageBox::warning(this, "权限不足", "您没有访问权限管理的权限");
        return;
    }

    if (!g_cliHandler) {
        QMessageBox::warning(this, "错误", "系统未初始化");
        return;
    }

    PermissionDialog dialog(g_cliHandler, this);
    dialog.exec();
}

void MainWindow::showErrorDialog(const QString& title, const QString& message)
{
    // 创建自定义消息框以支持更长的错误消息
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    //msgBox.setInformativeText(message);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    
    // 设置最小宽度以确保消息完整显示
    msgBox.setMinimumWidth(400);
    msgBox.adjustSize();
    
    msgBox.exec();
}
