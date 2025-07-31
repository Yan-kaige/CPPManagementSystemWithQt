#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QTreeWidget>
#include <QSplitter>
#include "Common.h"

class CLIHandler;

class PermissionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PermissionDialog(CLIHandler* cliHandler, QWidget *parent = nullptr);
    ~PermissionDialog();

private slots:
    void onRoleSelectionChanged();
    void onUserSelectionChanged();
    void onCreateRole();
    void onEditRole();
    void onDeleteRole();
    void onAssignRole();
    void onRemoveRole();
    void onMenuPermissionChanged();
    void refreshRoleList();
    void refreshUserList();
    void refreshMenuTree();

private:
    void setupUI();
    void setupRoleManagementTab();
    void setupUserRoleTab();
    void setupMenuPermissionTab();
    void loadRoles();
    void loadUsers();
    void loadMenus();
    void loadUserRoles(int userId);
    void loadRoleMenus(int roleId);
    void updateRoleMenuPermissions();
    
    // 安全的字符串转换函数
    QString safeFromStdString(const std::string& str);

    CLIHandler* m_cliHandler;
    
    // UI组件
    QTabWidget* m_tabWidget;
    
    // 角色管理页面
    QWidget* m_roleManagementWidget;
    QTableWidget* m_roleTable;
    QPushButton* m_btnCreateRole;
    QPushButton* m_btnEditRole;
    QPushButton* m_btnDeleteRole;
    QLineEdit* m_roleNameEdit;
    QLineEdit* m_roleCodeEdit;
    QTextEdit* m_roleDescEdit;
    
    // 用户角色分配页面
    QWidget* m_userRoleWidget;
    QTableWidget* m_userTable;
    QTableWidget* m_userRoleTable;
    QPushButton* m_btnAssignRole;
    QPushButton* m_btnRemoveRole;
    QComboBox* m_roleComboBox;
    
    // 菜单权限管理页面
    QWidget* m_menuPermissionWidget;
    QSplitter* m_menuSplitter;
    QTableWidget* m_roleListTable;
    QTreeWidget* m_menuTree;
    
    // 数据缓存
    std::vector<Role> m_roles;
    std::vector<User> m_users;
    std::vector<MenuItem> m_menus;
    int m_selectedUserId;
    int m_selectedRoleId;
};

// 角色编辑对话框
class RoleEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RoleEditDialog(const Role* role = nullptr, QWidget *parent = nullptr);
    
    Role getRole() const;
    bool isValid() const;

private slots:
    void onAccept();

private:
    void setupUI();
    
    QLineEdit* m_nameEdit;
    QLineEdit* m_codeEdit;
    QTextEdit* m_descEdit;
    QCheckBox* m_activeCheck;
    
    Role m_role;
    bool m_isEdit;
};
