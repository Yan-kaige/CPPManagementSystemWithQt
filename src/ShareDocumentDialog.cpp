#include "ShareDocumentDialog.h"
#include "CLIHandler.h"
#include <QGridLayout>
#include <QSplitter>

extern CLIHandler* g_cliHandler;

ShareDocumentDialog::ShareDocumentDialog(const Document& document, QWidget* parent)
    : QDialog(parent), currentDocument(document), selectedUserId(-1)
{
    setWindowTitle("分享文档");
    setModal(true);
    resize(600, 400);
    
    setupUI();
}

void ShareDocumentDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 文档信息显示
    documentInfoLabel = new QLabel(this);
    documentInfoLabel->setText(QString("分享文档: %1").arg(QString::fromUtf8(currentDocument.title)));
    documentInfoLabel->setStyleSheet("font-weight: bold; font-size: 14px; margin: 10px;");
    mainLayout->addWidget(documentInfoLabel);
    
    // 搜索区域
    QHBoxLayout* searchLayout = new QHBoxLayout();
    QLabel* searchLabel = new QLabel("搜索用户:", this);
    searchLineEdit = new QLineEdit(this);
    searchLineEdit->setPlaceholderText("输入用户名或邮箱");
    searchButton = new QPushButton("搜索", this);
    
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(searchLineEdit);
    searchLayout->addWidget(searchButton);
    
    mainLayout->addLayout(searchLayout);
    
    // 用户列表
    userTableWidget = new QTableWidget(this);
    userTableWidget->setColumnCount(4);
    QStringList headers;
    headers << "选择" << "ID" << "用户名" << "邮箱";
    userTableWidget->setHorizontalHeaderLabels(headers);
    userTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    userTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    userTableWidget->horizontalHeader()->setStretchLastSection(true);
    
    mainLayout->addWidget(userTableWidget);
    
    // 按钮区域
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    shareButton = new QPushButton("分享", this);
    cancelButton = new QPushButton("取消", this);
    shareButton->setEnabled(false);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(shareButton);
    buttonLayout->addWidget(cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // 连接信号
    connect(searchButton, &QPushButton::clicked, this, &ShareDocumentDialog::onSearchUserClicked);
    connect(searchLineEdit, &QLineEdit::returnPressed, this, &ShareDocumentDialog::onSearchUserClicked);
    connect(shareButton, &QPushButton::clicked, this, &ShareDocumentDialog::onShareClicked);
    connect(cancelButton, &QPushButton::clicked, this, &ShareDocumentDialog::onCancelClicked);
    connect(userTableWidget, &QTableWidget::itemSelectionChanged, [this]() {
        int currentRow = userTableWidget->currentRow();
        if (currentRow >= 0 && currentRow < currentUsers.size()) {
            selectedUserId = currentUsers[currentRow].id;
            shareButton->setEnabled(true);
        } else {
            selectedUserId = -1;
            shareButton->setEnabled(false);
        }
    });
}

void ShareDocumentDialog::onSearchUserClicked()
{
    QString keyword = searchLineEdit->text().trimmed();
    searchUsers(keyword);
}

void ShareDocumentDialog::onShareClicked()
{
    if (selectedUserId <= 0) {
        QMessageBox::warning(this, "警告", "请先选择要分享的用户");
        return;
    }
    
    // 找到选中的用户
    User selectedUser;
    bool found = false;
    for (const auto& user : currentUsers) {
        if (user.id == selectedUserId) {
            selectedUser = user;
            found = true;
            break;
        }
    }
    
    if (!found) {
        QMessageBox::warning(this, "错误", "选中的用户无效");
        return;
    }
    
    // 确认分享
    int ret = QMessageBox::question(this, "确认分享", 
        QString("确定要将文档 \"%1\" 分享给用户 \"%2\" 吗？")
        .arg(QString::fromUtf8(currentDocument.title))
        .arg(QString::fromUtf8(selectedUser.username)));
    
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    // 执行分享
    if (!g_cliHandler) {
        QMessageBox::critical(this, "错误", "CLIHandler 未初始化");
        return;
    }
    
    std::vector<std::string> args = {
        "sharedoc", 
        std::to_string(currentDocument.id), 
        selectedUser.username
    };
    
    bool success = g_cliHandler->handleShareDocument(args);
    
    if (success) {
        QMessageBox::information(this, "分享成功", 
            QString("文档已成功分享给用户 \"%1\"").arg(QString::fromUtf8(selectedUser.username)));
        accept();
    } else {
        QMessageBox::warning(this, "分享失败", "分享过程中出现错误，请检查日志");
    }
}

void ShareDocumentDialog::onCancelClicked()
{
    reject();
}

void ShareDocumentDialog::searchUsers(const QString& keyword)
{
    if (!g_cliHandler) {
        QMessageBox::critical(this, "错误", "CLIHandler 未初始化");
        return;
    }
    
    std::vector<User> users;
    if (keyword.isEmpty()) {
        users = g_cliHandler->getAllUsersForUI();
    } else {
        users = g_cliHandler->getSearchedUsersForUI(keyword.toUtf8().constData());
    }
    
    // 过滤掉当前用户
    auto currentUserResult = g_cliHandler->getCurrentUserForUI();
    if (currentUserResult.first) {
        int currentUserId = currentUserResult.second.id;
        users.erase(std::remove_if(users.begin(), users.end(), 
            [currentUserId](const User& user) {
                return user.id == currentUserId;
            }), users.end());
    }
    
    updateUserTable(users);
}

void ShareDocumentDialog::updateUserTable(const std::vector<User>& users)
{
    currentUsers = users;
    selectedUserId = -1;
    shareButton->setEnabled(false);
    
    userTableWidget->setRowCount(users.size());
    
    for (int i = 0; i < users.size(); ++i) {
        const User& user = users[i];
        
        // 选择列 - 空的，通过行选择来实现
        userTableWidget->setItem(i, 0, new QTableWidgetItem(""));
        userTableWidget->setItem(i, 1, new QTableWidgetItem(QString::number(user.id)));
        userTableWidget->setItem(i, 2, new QTableWidgetItem(QString::fromUtf8(user.username)));
        userTableWidget->setItem(i, 3, new QTableWidgetItem(QString::fromUtf8(user.email)));
    }
    
    userTableWidget->resizeColumnsToContents();
}
