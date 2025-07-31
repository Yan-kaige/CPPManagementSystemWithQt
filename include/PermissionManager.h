#pragma once

#include "Common.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>

// 前向声明
class DatabaseManager;

// 用户权限信息结构体
struct UserPermission {
    int user_id;
    std::string username;
    std::vector<Role> roles;
    std::unordered_set<std::string> permission_keys;  // 用户拥有的所有权限标识
    std::vector<MenuItem> accessible_menus;  // 用户可访问的菜单列表
};

// 权限管理器类
class PermissionManager {
private:
    DatabaseManager* dbManager;

    // 缓存
    std::unordered_map<int, MenuItem> menuCache;
    std::unordered_map<int, Role> roleCache;
    std::unordered_map<int, UserPermission> userPermissionCache;

    // 私有方法
    void clearCache();
    
public:
    explicit PermissionManager(DatabaseManager* dbManager);
    ~PermissionManager();
    
    // 角色管理
    Result<Role> createRole(const std::string& roleName, const std::string& roleCode, 
                           const std::string& description, bool isSystem = false);
    Result<Role> getRoleById(int roleId);
    Result<Role> getRoleByCode(const std::string& roleCode);
    Result<std::vector<Role>> getAllRoles();
    Result<bool> updateRole(const Role& role);
    Result<bool> deleteRole(int roleId);
    
    // 菜单管理
    Result<MenuItem> createMenu(const std::string& name, const std::string& code,
                               int parentId, MenuType type, const std::string& url = "",
                               const std::string& icon = "", const std::string& permissionKey = "",
                               ButtonType buttonType = ButtonType::CUSTOM, int sortOrder = 0,
                               bool isVisible = true, const std::string& description = "");
    Result<MenuItem> getMenuById(int menuId);
    Result<MenuItem> getMenuByCode(const std::string& code);
    Result<std::vector<MenuItem>> getAllMenus();
    Result<bool> updateMenu(const MenuItem& menu);
    Result<bool> deleteMenu(int menuId);
    
    // 用户角色关联管理
    Result<bool> assignRoleToUser(int userId, int roleId, int assignedBy = 0);
    Result<bool> removeRoleFromUser(int userId, int roleId);
    Result<std::vector<Role>> getUserRoles(int userId);
    Result<std::vector<User>> getRoleUsers(int roleId);
    
    // 角色权限管理
    Result<bool> grantMenuToRole(int roleId, int menuId);
    Result<bool> revokeMenuFromRole(int roleId, int menuId);
    Result<std::vector<MenuItem>> getRoleMenus(int roleId);
    Result<std::vector<Role>> getMenuRoles(int menuId);
    
    // 权限检查
    Result<UserPermission> getUserPermissions(int userId);
    Result<bool> hasPermission(int userId, const std::string& permissionKey);
    Result<bool> hasMenuAccess(int userId, const std::string& menuCode);
    Result<bool> hasButtonAccess(int userId, const std::string& buttonCode);
    
    // 批量操作
    Result<bool> batchAssignRolesToUser(int userId, const std::vector<int>& roleIds, int assignedBy = 0);
    Result<bool> batchGrantMenusToRole(int roleId, const std::vector<int>& menuIds);
    
    // 缓存管理
    void refreshCache();
    void refreshUserPermissionCache(int userId);
    
    // 工具方法
    std::string menuTypeToString(MenuType type);
    MenuType stringToMenuType(const std::string& typeStr);
    std::string buttonTypeToString(ButtonType type);
    ButtonType stringToButtonType(const std::string& typeStr);
    
    // 权限验证辅助方法
    static bool isValidPermissionKey(const std::string& permissionKey);
    static std::vector<std::string> parsePermissionKey(const std::string& permissionKey);
};
