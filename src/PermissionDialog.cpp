#include "PermissionDialog.h"
#include "CLIHandler.h"
#include "PermissionMacros.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QGroupBox>
#include <QGridLayout>
#include <QFormLayout>

PermissionDialog::PermissionDialog(CLIHandler* cliHandler, QWidget *parent)
    : QDialog(parent)
    , m_cliHandler(cliHandler)
    , m_selectedUserId(0)
    , m_selectedRoleId(0)
{
    setWindowTitle("权限管理");
    setMinimumSize(800, 600);
    setupUI();
    
    // 加载数据
    loadRoles();
    loadUsers();
    loadMenus();
}

PermissionDialog::~PermissionDialog()
{
}

void PermissionDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    m_tabWidget = new QTabWidget(this);
    
    setupRoleManagementTab();
    setupUserRoleTab();
    setupMenuPermissionTab();
    
    mainLayout->addWidget(m_tabWidget);
    
    // 底部按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* btnClose = new QPushButton("关闭", this);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addStretch();
    buttonLayout->addWidget(btnClose);
    
    mainLayout->addLayout(buttonLayout);
}

void PermissionDialog::setupRoleManagementTab()
{
    m_roleManagementWidget = new QWidget();
    m_tabWidget->addTab(m_roleManagementWidget, "角色管理");
    
    QHBoxLayout* layout = new QHBoxLayout(m_roleManagementWidget);
    
    // 左侧角色列表
    QVBoxLayout* leftLayout = new QVBoxLayout();
    
    QLabel* roleListLabel = new QLabel("角色列表:");
    leftLayout->addWidget(roleListLabel);
    
    m_roleTable = new QTableWidget();
    m_roleTable->setColumnCount(4);
    QStringList headers = {"ID", "角色名称", "角色编码", "描述"};
    m_roleTable->setHorizontalHeaderLabels(headers);
    m_roleTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_roleTable->horizontalHeader()->setStretchLastSection(true);
    leftLayout->addWidget(m_roleTable);
    
    // 角色操作按钮
    QHBoxLayout* roleButtonLayout = new QHBoxLayout();
    m_btnCreateRole = new QPushButton("创建角色");
    m_btnEditRole = new QPushButton("编辑角色");
    m_btnDeleteRole = new QPushButton("删除角色");
    
    roleButtonLayout->addWidget(m_btnCreateRole);
    roleButtonLayout->addWidget(m_btnEditRole);
    roleButtonLayout->addWidget(m_btnDeleteRole);
    roleButtonLayout->addStretch();
    
    leftLayout->addLayout(roleButtonLayout);
    
    layout->addLayout(leftLayout, 2);
    
    // 连接信号
    connect(m_roleTable, &QTableWidget::itemSelectionChanged, this, &PermissionDialog::onRoleSelectionChanged);
    connect(m_btnCreateRole, &QPushButton::clicked, this, &PermissionDialog::onCreateRole);
    connect(m_btnEditRole, &QPushButton::clicked, this, &PermissionDialog::onEditRole);
    connect(m_btnDeleteRole, &QPushButton::clicked, this, &PermissionDialog::onDeleteRole);
}

void PermissionDialog::setupUserRoleTab()
{
    m_userRoleWidget = new QWidget();
    m_tabWidget->addTab(m_userRoleWidget, "用户角色分配");
    
    QHBoxLayout* layout = new QHBoxLayout(m_userRoleWidget);
    
    // 左侧用户列表
    QVBoxLayout* leftLayout = new QVBoxLayout();
    QLabel* userListLabel = new QLabel("用户列表:");
    leftLayout->addWidget(userListLabel);
    
    m_userTable = new QTableWidget();
    m_userTable->setColumnCount(3);
    QStringList userHeaders = {"ID", "用户名", "邮箱"};
    m_userTable->setHorizontalHeaderLabels(userHeaders);
    m_userTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    leftLayout->addWidget(m_userTable);
    
    layout->addLayout(leftLayout, 1);
    
    // 右侧角色分配
    QVBoxLayout* rightLayout = new QVBoxLayout();
    QLabel* roleAssignLabel = new QLabel("用户角色:");
    rightLayout->addWidget(roleAssignLabel);
    
    m_userRoleTable = new QTableWidget();
    m_userRoleTable->setColumnCount(3);
    QStringList roleHeaders = {"角色ID", "角色名称", "分配时间"};
    m_userRoleTable->setHorizontalHeaderLabels(roleHeaders);
    rightLayout->addWidget(m_userRoleTable);
    
    // 角色分配操作
    QHBoxLayout* assignLayout = new QHBoxLayout();
    m_roleComboBox = new QComboBox();
    m_btnAssignRole = new QPushButton("分配角色");
    m_btnRemoveRole = new QPushButton("移除角色");
    
    assignLayout->addWidget(new QLabel("选择角色:"));
    assignLayout->addWidget(m_roleComboBox);
    assignLayout->addWidget(m_btnAssignRole);
    assignLayout->addWidget(m_btnRemoveRole);
    assignLayout->addStretch();
    
    rightLayout->addLayout(assignLayout);
    
    layout->addLayout(rightLayout, 1);
    
    // 连接信号
    connect(m_userTable, &QTableWidget::itemSelectionChanged, this, &PermissionDialog::onUserSelectionChanged);
    connect(m_btnAssignRole, &QPushButton::clicked, this, &PermissionDialog::onAssignRole);
    connect(m_btnRemoveRole, &QPushButton::clicked, this, &PermissionDialog::onRemoveRole);
}

void PermissionDialog::setupMenuPermissionTab()
{
    m_menuPermissionWidget = new QWidget();
    m_tabWidget->addTab(m_menuPermissionWidget, "菜单权限管理");
    
    QHBoxLayout* layout = new QHBoxLayout(m_menuPermissionWidget);
    
    m_menuSplitter = new QSplitter(Qt::Horizontal);
    
    // 左侧角色列表
    QWidget* roleWidget = new QWidget();
    QVBoxLayout* roleLayout = new QVBoxLayout(roleWidget);
    
    QLabel* roleLabel = new QLabel("角色列表:");
    roleLayout->addWidget(roleLabel);
    
    m_roleListTable = new QTableWidget();
    m_roleListTable->setColumnCount(2);
    QStringList roleHeaders = {"角色名称", "角色编码"};
    m_roleListTable->setHorizontalHeaderLabels(roleHeaders);
    m_roleListTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    roleLayout->addWidget(m_roleListTable);
    
    m_menuSplitter->addWidget(roleWidget);
    
    // 右侧菜单权限树
    QWidget* menuWidget = new QWidget();
    QVBoxLayout* menuLayout = new QVBoxLayout(menuWidget);
    
    QLabel* menuLabel = new QLabel("菜单权限:");
    menuLayout->addWidget(menuLabel);
    
    m_menuTree = new QTreeWidget();
    m_menuTree->setHeaderLabels({"菜单名称", "权限"});
    m_menuTree->setColumnWidth(0, 200);
    menuLayout->addWidget(m_menuTree);
    
    m_menuSplitter->addWidget(menuWidget);
    m_menuSplitter->setSizes({300, 500});
    
    layout->addWidget(m_menuSplitter);
    
    // 连接信号
    connect(m_roleListTable, &QTableWidget::itemSelectionChanged, this, &PermissionDialog::onRoleSelectionChanged);
}

void PermissionDialog::loadRoles()
{
    if (!m_cliHandler) return;
    
    auto result = m_cliHandler->getAllRoles();
    if (result.success) {
        m_roles = result.data.value();
        refreshRoleList();
    } else {
        QMessageBox::warning(this, "错误", QString("加载角色失败: %1").arg(QString::fromUtf8(result.message.c_str())));
    }
}

void PermissionDialog::loadUsers()
{
    if (!m_cliHandler) return;
    
    // 这里需要添加获取所有用户的方法
    // auto result = m_cliHandler->getAllUsers();
    // if (result.isSuccess()) {
    //     m_users = result.getValue();
    //     refreshUserList();
    // }
}

void PermissionDialog::loadMenus()
{
    if (!m_cliHandler) return;
    
    auto result = m_cliHandler->getAllMenus();
    if (result.success) {
        m_menus = result.data.value();
        refreshMenuTree();
    } else {
        QMessageBox::warning(this, "错误", QString("加载菜单失败: %1").arg(QString::fromUtf8(result.message.c_str())));
    }
}

void PermissionDialog::refreshRoleList()
{
    m_roleTable->setRowCount(m_roles.size());
    
    for (size_t i = 0; i < m_roles.size(); ++i) {
        const Role& role = m_roles[i];
        m_roleTable->setItem(i, 0, new QTableWidgetItem(QString::number(role.id)));
        m_roleTable->setItem(i, 1, new QTableWidgetItem(safeFromStdString(role.role_name)));
        m_roleTable->setItem(i, 2, new QTableWidgetItem(safeFromStdString(role.role_code)));
        m_roleTable->setItem(i, 3, new QTableWidgetItem(safeFromStdString(role.description)));
    }
    
    // 更新角色下拉框
    m_roleComboBox->clear();
    for (const Role& role : m_roles) {
        m_roleComboBox->addItem(safeFromStdString(role.role_name), role.id);
    }
    
    // 更新菜单权限页面的角色列表
    m_roleListTable->setRowCount(m_roles.size());
    for (size_t i = 0; i < m_roles.size(); ++i) {
        const Role& role = m_roles[i];
        m_roleListTable->setItem(i, 0, new QTableWidgetItem(safeFromStdString(role.role_name)));
        m_roleListTable->setItem(i, 1, new QTableWidgetItem(safeFromStdString(role.role_code)));
        m_roleListTable->item(i, 0)->setData(Qt::UserRole, role.id);
    }
}

void PermissionDialog::refreshUserList()
{
    // 实现用户列表刷新
}

void PermissionDialog::refreshMenuTree()
{
    m_menuTree->clear();
    
    // 构建菜单树
    std::map<int, QTreeWidgetItem*> itemMap;
    
    for (const MenuItem& menu : m_menus) {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, safeFromStdString(menu.name));
        item->setData(0, Qt::UserRole, menu.id);
        
        // 添加权限复选框
        QCheckBox* checkBox = new QCheckBox();
        m_menuTree->setItemWidget(item, 1, checkBox);
        
        itemMap[menu.id] = item;
        
        if (menu.parent_id == 0) {
            m_menuTree->addTopLevelItem(item);
        } else {
            auto parentIt = itemMap.find(menu.parent_id);
            if (parentIt != itemMap.end()) {
                parentIt->second->addChild(item);
            }
        }
    }
    
    m_menuTree->expandAll();
}

// 槽函数实现
void PermissionDialog::onRoleSelectionChanged()
{
    QTableWidget* sender = qobject_cast<QTableWidget*>(QObject::sender());
    if (!sender) return;

    int currentRow = sender->currentRow();
    if (currentRow < 0) return;

    if (sender == m_roleTable) {
        // 角色管理页面的选择
        m_btnEditRole->setEnabled(true);
        m_btnDeleteRole->setEnabled(true);
    } else if (sender == m_roleListTable) {
        // 菜单权限页面的角色选择
        QTableWidgetItem* item = m_roleListTable->item(currentRow, 0);
        if (item) {
            m_selectedRoleId = item->data(Qt::UserRole).toInt();
            loadRoleMenus(m_selectedRoleId);
        }
    }
}

void PermissionDialog::onUserSelectionChanged()
{
    int currentRow = m_userTable->currentRow();
    if (currentRow < 0) return;

    QTableWidgetItem* item = m_userTable->item(currentRow, 0);
    if (item) {
        m_selectedUserId = item->text().toInt();
        loadUserRoles(m_selectedUserId);

        m_btnAssignRole->setEnabled(true);
        m_btnRemoveRole->setEnabled(true);
    }
}

void PermissionDialog::onCreateRole()
{
    // 检查权限
    if (!HAS_PERMISSION("role:add")) {
        QMessageBox::warning(this, "权限不足", "您没有创建角色的权限");
        return;
    }

    RoleEditDialog dialog(nullptr, this);
    if (dialog.exec() == QDialog::Accepted && dialog.isValid()) {
        Role newRole = dialog.getRole();

        auto result = m_cliHandler->createRole(newRole.role_name, newRole.role_code, newRole.description);
        if (result.success) {
            QMessageBox::information(this, "成功", "角色创建成功");
            loadRoles();
        } else {
            QMessageBox::warning(this, "错误", QString("创建角色失败: %1").arg(QString::fromUtf8(result.message.c_str())));
        }
    }
}

void PermissionDialog::onEditRole()
{
    // 检查权限
    if (!HAS_PERMISSION("role:edit")) {
        QMessageBox::warning(this, "权限不足", "您没有编辑角色的权限");
        return;
    }

    int currentRow = m_roleTable->currentRow();
    if (currentRow < 0 || currentRow >= static_cast<int>(m_roles.size())) return;

    const Role& role = m_roles[currentRow];
    RoleEditDialog dialog(&role, this);
    if (dialog.exec() == QDialog::Accepted && dialog.isValid()) {
        // 这里需要实现角色更新功能
        QMessageBox::information(this, "提示", "角色编辑功能待实现");
    }
}

void PermissionDialog::onDeleteRole()
{
    // 检查权限
    if (!HAS_PERMISSION("role:delete")) {
        QMessageBox::warning(this, "权限不足", "您没有删除角色的权限");
        return;
    }

    int currentRow = m_roleTable->currentRow();
    if (currentRow < 0 || currentRow >= static_cast<int>(m_roles.size())) return;

    const Role& role = m_roles[currentRow];

    if (role.is_system) {
        QMessageBox::warning(this, "错误", "不能删除系统内置角色");
        return;
    }

    int ret = QMessageBox::question(this, "确认删除",
        QString("确定要删除角色 '%1' 吗？").arg(safeFromStdString(role.role_name)));

    if (ret == QMessageBox::Yes) {
        // 这里需要实现角色删除功能
        QMessageBox::information(this, "提示", "角色删除功能待实现");
    }
}

void PermissionDialog::onAssignRole()
{
    if (m_selectedUserId == 0) return;

    int roleId = m_roleComboBox->currentData().toInt();
    if (roleId == 0) return;

    auto result = m_cliHandler->assignRoleToUser(m_selectedUserId, roleId);
    if (result.success) {
        QMessageBox::information(this, "成功", "角色分配成功");
        loadUserRoles(m_selectedUserId);
    } else {
        QMessageBox::warning(this, "错误", QString("分配角色失败: %1").arg(QString::fromUtf8(result.message.c_str())));
    }
}

void PermissionDialog::onRemoveRole()
{
    if (m_selectedUserId == 0) return;

    int currentRow = m_userRoleTable->currentRow();
    if (currentRow < 0) return;

    QTableWidgetItem* item = m_userRoleTable->item(currentRow, 0);
    if (!item) return;

    int roleId = item->text().toInt();

    auto result = m_cliHandler->removeRoleFromUser(m_selectedUserId, roleId);
    if (result.success) {
        QMessageBox::information(this, "成功", "角色移除成功");
        loadUserRoles(m_selectedUserId);
    } else {
        QMessageBox::warning(this, "错误", QString("移除角色失败: %1").arg(QString::fromUtf8(result.message.c_str())));
    }
}

void PermissionDialog::onMenuPermissionChanged()
{
    // 实现菜单权限变更
}

void PermissionDialog::loadUserRoles(int userId)
{
    if (!m_cliHandler) return;

    auto result = m_cliHandler->getUserRoles(userId);
    if (result.success) {
        const auto& roles = result.data.value();

        m_userRoleTable->setRowCount(roles.size());
        for (size_t i = 0; i < roles.size(); ++i) {
            const Role& role = roles[i];
            m_userRoleTable->setItem(i, 0, new QTableWidgetItem(QString::number(role.id)));
            m_userRoleTable->setItem(i, 1, new QTableWidgetItem(safeFromStdString(role.role_name)));
            m_userRoleTable->setItem(i, 2, new QTableWidgetItem("分配时间")); // 需要实现时间显示
        }
    }
}

void PermissionDialog::loadRoleMenus(int roleId)
{
    // 实现角色菜单权限加载
}

void PermissionDialog::updateRoleMenuPermissions()
{
    // 实现角色菜单权限更新
}

// RoleEditDialog 实现
RoleEditDialog::RoleEditDialog(const Role* role, QWidget *parent)
    : QDialog(parent)
    , m_isEdit(role != nullptr)
{
    if (m_isEdit) {
        m_role = *role;
        setWindowTitle("编辑角色");
    } else {
        setWindowTitle("创建角色");
        m_role.id = 0;
        m_role.is_active = true;
        m_role.is_system = false;
    }

    setupUI();
}

void RoleEditDialog::setupUI()
{
    setMinimumSize(400, 300);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QFormLayout* formLayout = new QFormLayout();

    m_nameEdit = new QLineEdit();
    m_nameEdit->setText(QString::fromUtf8(m_role.role_name.c_str()));
    formLayout->addRow("角色名称:", m_nameEdit);

    m_codeEdit = new QLineEdit();
    m_codeEdit->setText(QString::fromUtf8(m_role.role_code.c_str()));
    if (m_isEdit && m_role.is_system) {
        m_codeEdit->setEnabled(false); // 系统角色不能修改编码
    }
    formLayout->addRow("角色编码:", m_codeEdit);

    m_descEdit = new QTextEdit();
    m_descEdit->setPlainText(QString::fromUtf8(m_role.description.c_str()));
    m_descEdit->setMaximumHeight(100);
    formLayout->addRow("描述:", m_descEdit);

    m_activeCheck = new QCheckBox("启用");
    m_activeCheck->setChecked(m_role.is_active);
    formLayout->addRow("状态:", m_activeCheck);

    mainLayout->addLayout(formLayout);

    // 按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* btnOk = new QPushButton("确定");
    QPushButton* btnCancel = new QPushButton("取消");

    buttonLayout->addStretch();
    buttonLayout->addWidget(btnOk);
    buttonLayout->addWidget(btnCancel);

    mainLayout->addLayout(buttonLayout);

    connect(btnOk, &QPushButton::clicked, this, &RoleEditDialog::onAccept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

void RoleEditDialog::onAccept()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "错误", "角色名称不能为空");
        return;
    }

    if (m_codeEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "错误", "角色编码不能为空");
        return;
    }

    m_role.role_name = m_nameEdit->text().trimmed().toStdString();
    m_role.role_code = m_codeEdit->text().trimmed().toStdString();
    m_role.description = m_descEdit->toPlainText().toStdString();
    m_role.is_active = m_activeCheck->isChecked();

    accept();
}

Role RoleEditDialog::getRole() const
{
    return m_role;
}

bool RoleEditDialog::isValid() const
{
    return !m_role.role_name.empty() && !m_role.role_code.empty();
}

QString PermissionDialog::safeFromStdString(const std::string& str)
{
    try {
        // 检查字符串是否为空或包含无效字符
        if (str.empty()) {
            return QString();
        }
        
        // 检查字符串是否包含空字符
        if (str.find('\0') != std::string::npos) {
            // 如果包含空字符，截取到第一个空字符
            size_t nullPos = str.find('\0');
            std::string safeStr = str.substr(0, nullPos);
            return QString::fromUtf8(safeStr.c_str());
        }
        
        // 使用UTF-8转换，这是最安全的方式
        return QString::fromUtf8(str.c_str());
    } catch (...) {
        // 如果转换失败，返回空字符串
        return QString();
    }
}
