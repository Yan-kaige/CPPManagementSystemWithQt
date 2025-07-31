#include "PermissionManager.h"
#include "DatabaseManager.h"
#include "Logger.h"
#include <sstream>
#include <algorithm>

PermissionManager::PermissionManager(DatabaseManager* dbManager) 
    : dbManager(dbManager) {
    if (!dbManager) {
        LOG_ERROR("DatabaseManager 指针为空");
    }
}

PermissionManager::~PermissionManager() {
    clearCache();
}

void PermissionManager::clearCache() {
    menuCache.clear();
    roleCache.clear();
    userPermissionCache.clear();
}

std::string PermissionManager::menuTypeToString(MenuType type) {
    switch (type) {
        case MenuType::DIRECTORY: return "DIRECTORY";
        case MenuType::MENU: return "MENU";
        case MenuType::BUTTON: return "BUTTON";
        default: return "MENU";
    }
}

MenuType PermissionManager::stringToMenuType(const std::string& typeStr) {
    if (typeStr == "DIRECTORY") return MenuType::DIRECTORY;
    if (typeStr == "MENU") return MenuType::MENU;
    if (typeStr == "BUTTON") return MenuType::BUTTON;
    return MenuType::MENU;
}

std::string PermissionManager::buttonTypeToString(ButtonType type) {
    switch (type) {
        case ButtonType::ADD: return "ADD";
        case ButtonType::EDIT: return "EDIT";
        case ButtonType::DELETE_BTN: return "DELETE";
        case ButtonType::VIEW: return "VIEW";
        case ButtonType::EXPORT: return "EXPORT";
        case ButtonType::IMPORT: return "IMPORT";
        case ButtonType::CUSTOM: return "CUSTOM";
        default: return "CUSTOM";
    }
}

ButtonType PermissionManager::stringToButtonType(const std::string& typeStr) {
    if (typeStr == "ADD") return ButtonType::ADD;
    if (typeStr == "EDIT") return ButtonType::EDIT;
    if (typeStr == "DELETE") return ButtonType::DELETE_BTN;
    if (typeStr == "VIEW") return ButtonType::VIEW;
    if (typeStr == "EXPORT") return ButtonType::EXPORT;
    if (typeStr == "IMPORT") return ButtonType::IMPORT;
    if (typeStr == "CUSTOM") return ButtonType::CUSTOM;
    return ButtonType::CUSTOM;
}

// 角色管理
Result<Role> PermissionManager::createRole(const std::string& roleName, const std::string& roleCode, 
                                         const std::string& description, bool isSystem) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<Role>::Error("数据库未连接");
    }

    std::string sql = "INSERT INTO roles (role_name, role_code, description, is_system) VALUES ('" +
                      roleName + "', '" + roleCode + "', '" + description + "', " +
                      (isSystem ? "TRUE" : "FALSE") + ");";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<Role>::Error("创建角色失败: " + result.message);
    }

    // 获取新创建的角色
    return getRoleByCode(roleCode);
}

Result<Role> PermissionManager::getRoleById(int roleId) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<Role>::Error("数据库未连接");
    }

    // 检查缓存
    auto it = roleCache.find(roleId);
    if (it != roleCache.end()) {
        return Result<Role>::Success(it->second);
    }

    std::string sql = "SELECT id, role_name, role_code, description, is_active, is_system, "
                      "created_at, updated_at, created_by, updated_by FROM roles WHERE id = " +
                      std::to_string(roleId) + ";";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<Role>::Error("查询角色失败: " + result.message);
    }

    auto rows = result.data.value();
    if (rows.empty()) {
        return Result<Role>::Error("角色不存在");
    }

    Role role;
    auto& row = rows[0];
    role.id = std::stoi(row["id"]);
    role.role_name = row["role_name"];
    role.role_code = row["role_code"];
    role.description = row["description"];
    role.is_active = row["is_active"] == "1";
    role.is_system = row["is_system"] == "1";
    role.created_by = row["created_by"].empty() ? 0 : std::stoi(row["created_by"]);
    role.updated_by = row["updated_by"].empty() ? 0 : std::stoi(row["updated_by"]);
    // TODO: 解析时间字段
    role.created_at = std::chrono::system_clock::now();
    role.updated_at = std::chrono::system_clock::now();

    // 缓存结果
    roleCache[roleId] = role;

    return Result<Role>::Success(role);
}

Result<Role> PermissionManager::getRoleByCode(const std::string& roleCode) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<Role>::Error("数据库未连接");
    }

    std::string sql = "SELECT id, role_name, role_code, description, is_active, is_system, "
                      "created_at, updated_at, created_by, updated_by FROM roles WHERE role_code = '" +
                      roleCode + "';";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<Role>::Error("查询角色失败: " + result.message);
    }

    auto rows = result.data.value();
    if (rows.empty()) {
        return Result<Role>::Error("角色不存在");
    }

    Role role;
    auto& row = rows[0];
    role.id = std::stoi(row["id"]);
    role.role_name = row["role_name"];
    role.role_code = row["role_code"];
    role.description = row["description"];
    role.is_active = row["is_active"] == "1";
    role.is_system = row["is_system"] == "1";
    role.created_by = row["created_by"].empty() ? 0 : std::stoi(row["created_by"]);
    role.updated_by = row["updated_by"].empty() ? 0 : std::stoi(row["updated_by"]);
    role.created_at = std::chrono::system_clock::now();
    role.updated_at = std::chrono::system_clock::now();

    // 缓存结果
    roleCache[role.id] = role;

    return Result<Role>::Success(role);
}

Result<std::vector<Role>> PermissionManager::getAllRoles() {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<std::vector<Role>>::Error("数据库未连接");
    }

    std::string sql = "SELECT id, role_name, role_code, description, is_active, is_system, "
                      "created_at, updated_at, created_by, updated_by FROM roles "
                      "WHERE is_active = TRUE ORDER BY role_name;";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<std::vector<Role>>::Error("查询角色列表失败: " + result.message);
    }

    std::vector<Role> roles;
    for (const auto& row : result.data.value()) {
        Role role;
        role.id = std::stoi(row.at("id"));
        role.role_name = row.at("role_name");
        role.role_code = row.at("role_code");
        role.description = row.at("description");
        role.is_active = row.at("is_active") == "1";
        role.is_system = row.at("is_system") == "1";
        role.created_by = row.at("created_by").empty() ? 0 : std::stoi(row.at("created_by"));
        role.updated_by = row.at("updated_by").empty() ? 0 : std::stoi(row.at("updated_by"));
        role.created_at = std::chrono::system_clock::now();
        role.updated_at = std::chrono::system_clock::now();

        roles.push_back(role);
        
        // 缓存结果
        roleCache[role.id] = role;
    }

    return Result<std::vector<Role>>::Success(roles);
}

Result<std::vector<MenuItem>> PermissionManager::getAllMenus() {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<std::vector<MenuItem>>::Error("数据库未连接");
    }

    std::string sql = "SELECT id, name, code, parent_id, type, url, icon, permission_key, "
                      "button_type, sort_order, is_visible, is_active, description, "
                      "created_at, updated_at, created_by, updated_by FROM menus "
                      "WHERE is_active = TRUE ORDER BY sort_order, name;";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<std::vector<MenuItem>>::Error("查询菜单列表失败: " + result.message);
    }

    std::vector<MenuItem> menus;
    for (const auto& row : result.data.value()) {
        MenuItem menu;
        menu.id = std::stoi(row.at("id"));
        menu.name = row.at("name");
        menu.code = row.at("code");
        menu.parent_id = row.at("parent_id").empty() ? 0 : std::stoi(row.at("parent_id"));
        menu.type = stringToMenuType(row.at("type"));
        menu.url = row.at("url");
        menu.icon = row.at("icon");
        menu.permission_key = row.at("permission_key");
        menu.button_type = stringToButtonType(row.at("button_type"));
        menu.sort_order = std::stoi(row.at("sort_order"));
        menu.is_visible = row.at("is_visible") == "1";
        menu.is_active = row.at("is_active") == "1";
        menu.description = row.at("description");
        menu.created_by = row.at("created_by").empty() ? 0 : std::stoi(row.at("created_by"));
        menu.updated_by = row.at("updated_by").empty() ? 0 : std::stoi(row.at("updated_by"));
        menu.created_at = std::chrono::system_clock::now();
        menu.updated_at = std::chrono::system_clock::now();

        menus.push_back(menu);
    }

    return Result<std::vector<MenuItem>>::Success(menus);
}

Result<bool> PermissionManager::updateRole(const Role& role) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<bool>::Error("数据库未连接");
    }

    std::string sql = "UPDATE roles SET role_name = '" + role.role_name +
                      "', description = '" + role.description +
                      "', is_active = " + (role.is_active ? "TRUE" : "FALSE") +
                      ", updated_by = " + std::to_string(role.updated_by) +
                      " WHERE id = " + std::to_string(role.id) + ";";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<bool>::Error("更新角色失败: " + result.message);
    }

    // 清除缓存
    roleCache.erase(role.id);

    return Result<bool>::Success(true);
}

Result<bool> PermissionManager::deleteRole(int roleId) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<bool>::Error("数据库未连接");
    }

    // 检查是否为系统角色
    auto roleResult = getRoleById(roleId);
    if (roleResult.success && roleResult.data.has_value() && roleResult.data.value().is_system) {
        return Result<bool>::Error("不能删除系统内置角色");
    }

    std::string sql = "DELETE FROM roles WHERE id = " + std::to_string(roleId) + ";";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<bool>::Error("删除角色失败: " + result.message);
    }

    // 清除缓存
    roleCache.erase(roleId);

    return Result<bool>::Success(true);
}

// 用户角色关联管理
Result<bool> PermissionManager::assignRoleToUser(int userId, int roleId, int assignedBy) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<bool>::Error("数据库未连接");
    }

    std::string sql = "INSERT INTO user_roles (user_id, role_id, assigned_by) VALUES (" +
                      std::to_string(userId) + ", " + std::to_string(roleId) + ", " +
                      std::to_string(assignedBy) + ") ON DUPLICATE KEY UPDATE is_active = TRUE;";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<bool>::Error("分配角色失败: " + result.message);
    }

    // 清除用户权限缓存
    userPermissionCache.erase(userId);

    return Result<bool>::Success(true);
}

Result<bool> PermissionManager::removeRoleFromUser(int userId, int roleId) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<bool>::Error("数据库未连接");
    }

    std::string sql = "DELETE FROM user_roles WHERE user_id = " + std::to_string(userId) +
                      " AND role_id = " + std::to_string(roleId) + ";";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<bool>::Error("移除角色失败: " + result.message);
    }

    // 清除用户权限缓存
    userPermissionCache.erase(userId);

    return Result<bool>::Success(true);
}

Result<std::vector<Role>> PermissionManager::getUserRoles(int userId) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<std::vector<Role>>::Error("数据库未连接");
    }

    std::string sql = "SELECT r.id, r.role_name, r.role_code, r.description, r.is_active, "
                      "r.is_system, r.created_at, r.updated_at, r.created_by, r.updated_by "
                      "FROM roles r JOIN user_roles ur ON r.id = ur.role_id "
                      "WHERE ur.user_id = " + std::to_string(userId) + " AND ur.is_active = TRUE "
                      "AND r.is_active = TRUE AND (ur.expires_at IS NULL OR ur.expires_at > NOW());";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<std::vector<Role>>::Error("查询用户角色失败: " + result.message);
    }

    std::vector<Role> roles;
    for (const auto& row : result.data.value()) {
        Role role;
        role.id = std::stoi(row.at("id"));
        role.role_name = row.at("role_name");
        role.role_code = row.at("role_code");
        role.description = row.at("description");
        role.is_active = row.at("is_active") == "1";
        role.is_system = row.at("is_system") == "1";
        role.created_by = row.at("created_by").empty() ? 0 : std::stoi(row.at("created_by"));
        role.updated_by = row.at("updated_by").empty() ? 0 : std::stoi(row.at("updated_by"));
        role.created_at = std::chrono::system_clock::now();
        role.updated_at = std::chrono::system_clock::now();

        roles.push_back(role);
    }

    return Result<std::vector<Role>>::Success(roles);
}

// 角色权限管理
Result<bool> PermissionManager::grantMenuToRole(int roleId, int menuId) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<bool>::Error("数据库未连接");
    }

    std::string sql = "INSERT INTO role_menus (role_id, menu_id, is_granted) VALUES (" +
                      std::to_string(roleId) + ", " + std::to_string(menuId) + ", TRUE) "
                      "ON DUPLICATE KEY UPDATE is_granted = TRUE;";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<bool>::Error("授权菜单失败: " + result.message);
    }

    // 清除相关用户权限缓存
    clearCache();

    return Result<bool>::Success(true);
}

Result<bool> PermissionManager::revokeMenuFromRole(int roleId, int menuId) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<bool>::Error("数据库未连接");
    }

    std::string sql = "UPDATE role_menus SET is_granted = FALSE WHERE role_id = " +
                      std::to_string(roleId) + " AND menu_id = " + std::to_string(menuId) + ";";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<bool>::Error("撤销菜单权限失败: " + result.message);
    }

    // 清除相关用户权限缓存
    clearCache();

    return Result<bool>::Success(true);
}

// 权限检查
Result<bool> PermissionManager::hasPermission(int userId, const std::string& permissionKey) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<bool>::Error("数据库未连接");
    }

    std::string sql = "SELECT COUNT(*) as count FROM user_permissions "
                      "WHERE user_id = " + std::to_string(userId) +
                      " AND permission_key = '" + permissionKey + "';";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<bool>::Error("检查权限失败: " + result.message);
    }

    auto rows = result.data.value();
    if (rows.empty()) {
        return Result<bool>::Success(false);
    }

    int count = std::stoi(rows[0].at("count"));
    return Result<bool>::Success(count > 0);
}

Result<bool> PermissionManager::hasMenuAccess(int userId, const std::string& menuCode) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<bool>::Error("数据库未连接");
    }

    std::string sql = "SELECT COUNT(*) as count FROM user_permissions "
                      "WHERE user_id = " + std::to_string(userId) +
                      " AND menu_code = '" + menuCode + "';";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<bool>::Error("检查菜单权限失败: " + result.message);
    }

    auto rows = result.data.value();
    if (rows.empty()) {
        return Result<bool>::Success(false);
    }

    int count = std::stoi(rows[0].at("count"));
    return Result<bool>::Success(count > 0);
}

Result<bool> PermissionManager::hasButtonAccess(int userId, const std::string& buttonCode) {
    return hasMenuAccess(userId, buttonCode);
}

void PermissionManager::refreshCache() {
    clearCache();
}

void PermissionManager::refreshUserPermissionCache(int userId) {
    userPermissionCache.erase(userId);
}

// 菜单管理的其他方法
Result<MenuItem> PermissionManager::createMenu(const std::string& name, const std::string& code,
                                             int parentId, MenuType type, const std::string& url,
                                             const std::string& icon, const std::string& permissionKey,
                                             ButtonType buttonType, int sortOrder,
                                             bool isVisible, const std::string& description) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<MenuItem>::Error("数据库未连接");
    }

    std::string parentIdStr = (parentId == 0) ? "NULL" : std::to_string(parentId);
    std::string sql = "INSERT INTO menus (name, code, parent_id, type, url, icon, permission_key, "
                      "button_type, sort_order, is_visible, description) VALUES ('" +
                      name + "', '" + code + "', " + parentIdStr + ", '" + menuTypeToString(type) +
                      "', '" + url + "', '" + icon + "', '" + permissionKey + "', '" +
                      buttonTypeToString(buttonType) + "', " + std::to_string(sortOrder) + ", " +
                      (isVisible ? "TRUE" : "FALSE") + ", '" + description + "');";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<MenuItem>::Error("创建菜单失败: " + result.message);
    }

    return getMenuByCode(code);
}

Result<MenuItem> PermissionManager::getMenuById(int menuId) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<MenuItem>::Error("数据库未连接");
    }

    std::string sql = "SELECT id, name, code, parent_id, type, url, icon, permission_key, "
                      "button_type, sort_order, is_visible, is_active, description, "
                      "created_at, updated_at, created_by, updated_by FROM menus WHERE id = " +
                      std::to_string(menuId) + ";";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<MenuItem>::Error("查询菜单失败: " + result.message);
    }

    auto rows = result.data.value();
    if (rows.empty()) {
        return Result<MenuItem>::Error("菜单不存在");
    }

    MenuItem menu;
    auto& row = rows[0];
    menu.id = std::stoi(row["id"]);
    menu.name = row["name"];
    menu.code = row["code"];
    menu.parent_id = row["parent_id"].empty() ? 0 : std::stoi(row["parent_id"]);
    menu.type = stringToMenuType(row["type"]);
    menu.url = row["url"];
    menu.icon = row["icon"];
    menu.permission_key = row["permission_key"];
    menu.button_type = stringToButtonType(row["button_type"]);
    menu.sort_order = std::stoi(row["sort_order"]);
    menu.is_visible = row["is_visible"] == "1";
    menu.is_active = row["is_active"] == "1";
    menu.description = row["description"];
    menu.created_by = row["created_by"].empty() ? 0 : std::stoi(row["created_by"]);
    menu.updated_by = row["updated_by"].empty() ? 0 : std::stoi(row["updated_by"]);
    menu.created_at = std::chrono::system_clock::now();
    menu.updated_at = std::chrono::system_clock::now();

    return Result<MenuItem>::Success(menu);
}

Result<MenuItem> PermissionManager::getMenuByCode(const std::string& code) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<MenuItem>::Error("数据库未连接");
    }

    std::string sql = "SELECT id, name, code, parent_id, type, url, icon, permission_key, "
                      "button_type, sort_order, is_visible, is_active, description, "
                      "created_at, updated_at, created_by, updated_by FROM menus WHERE code = '" +
                      code + "';";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<MenuItem>::Error("查询菜单失败: " + result.message);
    }

    auto rows = result.data.value();
    if (rows.empty()) {
        return Result<MenuItem>::Error("菜单不存在");
    }

    MenuItem menu;
    auto& row = rows[0];
    menu.id = std::stoi(row["id"]);
    menu.name = row["name"];
    menu.code = row["code"];
    menu.parent_id = row["parent_id"].empty() ? 0 : std::stoi(row["parent_id"]);
    menu.type = stringToMenuType(row["type"]);
    menu.url = row["url"];
    menu.icon = row["icon"];
    menu.permission_key = row["permission_key"];
    menu.button_type = stringToButtonType(row["button_type"]);
    menu.sort_order = std::stoi(row["sort_order"]);
    menu.is_visible = row["is_visible"] == "1";
    menu.is_active = row["is_active"] == "1";
    menu.description = row["description"];
    menu.created_by = row["created_by"].empty() ? 0 : std::stoi(row["created_by"]);
    menu.updated_by = row["updated_by"].empty() ? 0 : std::stoi(row["updated_by"]);
    menu.created_at = std::chrono::system_clock::now();
    menu.updated_at = std::chrono::system_clock::now();

    return Result<MenuItem>::Success(menu);
}

// 其他缺失的方法实现
Result<bool> PermissionManager::updateMenu(const MenuItem& menu) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<bool>::Error("数据库未连接");
    }

    std::string sql = "UPDATE menus SET name = '" + menu.name +
                      "', url = '" + menu.url +
                      "', icon = '" + menu.icon +
                      "', permission_key = '" + menu.permission_key +
                      "', sort_order = " + std::to_string(menu.sort_order) +
                      ", is_visible = " + (menu.is_visible ? "TRUE" : "FALSE") +
                      ", description = '" + menu.description +
                      "' WHERE id = " + std::to_string(menu.id) + ";";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<bool>::Error("更新菜单失败: " + result.message);
    }

    return Result<bool>::Success(true);
}

Result<bool> PermissionManager::deleteMenu(int menuId) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<bool>::Error("数据库未连接");
    }

    std::string sql = "DELETE FROM menus WHERE id = " + std::to_string(menuId) + ";";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<bool>::Error("删除菜单失败: " + result.message);
    }

    return Result<bool>::Success(true);
}

Result<std::vector<User>> PermissionManager::getRoleUsers(int roleId) {
    // 这个方法需要User类的定义，暂时返回空实现
    return Result<std::vector<User>>::Error("方法未实现");
}

Result<std::vector<MenuItem>> PermissionManager::getRoleMenus(int roleId) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<std::vector<MenuItem>>::Error("数据库未连接");
    }

    std::string sql = "SELECT m.id, m.name, m.code, m.parent_id, m.type, m.url, m.icon, "
                      "m.permission_key, m.button_type, m.sort_order, m.is_visible, m.is_active, "
                      "m.description, m.created_at, m.updated_at, m.created_by, m.updated_by "
                      "FROM menus m JOIN role_menus rm ON m.id = rm.menu_id "
                      "WHERE rm.role_id = " + std::to_string(roleId) + " AND rm.is_granted = TRUE "
                      "AND m.is_active = TRUE ORDER BY m.sort_order, m.name;";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<std::vector<MenuItem>>::Error("查询角色菜单失败: " + result.message);
    }

    std::vector<MenuItem> menus;
    for (const auto& row : result.data.value()) {
        MenuItem menu;
        menu.id = std::stoi(row.at("id"));
        menu.name = row.at("name");
        menu.code = row.at("code");
        menu.parent_id = row.at("parent_id").empty() ? 0 : std::stoi(row.at("parent_id"));
        menu.type = stringToMenuType(row.at("type"));
        menu.url = row.at("url");
        menu.icon = row.at("icon");
        menu.permission_key = row.at("permission_key");
        menu.button_type = stringToButtonType(row.at("button_type"));
        menu.sort_order = std::stoi(row.at("sort_order"));
        menu.is_visible = row.at("is_visible") == "1";
        menu.is_active = row.at("is_active") == "1";
        menu.description = row.at("description");
        menu.created_by = row.at("created_by").empty() ? 0 : std::stoi(row.at("created_by"));
        menu.updated_by = row.at("updated_by").empty() ? 0 : std::stoi(row.at("updated_by"));
        menu.created_at = std::chrono::system_clock::now();
        menu.updated_at = std::chrono::system_clock::now();

        menus.push_back(menu);
    }

    return Result<std::vector<MenuItem>>::Success(menus);
}

Result<std::vector<Role>> PermissionManager::getMenuRoles(int menuId) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<std::vector<Role>>::Error("数据库未连接");
    }

    std::string sql = "SELECT r.id, r.role_name, r.role_code, r.description, r.is_active, "
                      "r.is_system, r.created_at, r.updated_at, r.created_by, r.updated_by "
                      "FROM roles r JOIN role_menus rm ON r.id = rm.role_id "
                      "WHERE rm.menu_id = " + std::to_string(menuId) + " AND rm.is_granted = TRUE "
                      "AND r.is_active = TRUE ORDER BY r.role_name;";

    auto result = dbManager->executeQuery(sql);
    if (!result.success) {
        return Result<std::vector<Role>>::Error("查询菜单角色失败: " + result.message);
    }

    std::vector<Role> roles;
    for (const auto& row : result.data.value()) {
        Role role;
        role.id = std::stoi(row.at("id"));
        role.role_name = row.at("role_name");
        role.role_code = row.at("role_code");
        role.description = row.at("description");
        role.is_active = row.at("is_active") == "1";
        role.is_system = row.at("is_system") == "1";
        role.created_by = row.at("created_by").empty() ? 0 : std::stoi(row.at("created_by"));
        role.updated_by = row.at("updated_by").empty() ? 0 : std::stoi(row.at("updated_by"));
        role.created_at = std::chrono::system_clock::now();
        role.updated_at = std::chrono::system_clock::now();

        roles.push_back(role);
    }

    return Result<std::vector<Role>>::Success(roles);
}

Result<UserPermission> PermissionManager::getUserPermissions(int userId) {
    // 这个方法需要更复杂的实现，暂时返回简单版本
    UserPermission userPerm;
    userPerm.user_id = userId;

    // 获取用户角色
    auto rolesResult = getUserRoles(userId);
    if (rolesResult.success) {
        userPerm.roles = rolesResult.data.value();
    }

    return Result<UserPermission>::Success(userPerm);
}

Result<bool> PermissionManager::batchAssignRolesToUser(int userId, const std::vector<int>& roleIds, int assignedBy) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<bool>::Error("数据库未连接");
    }

    for (int roleId : roleIds) {
        auto result = assignRoleToUser(userId, roleId, assignedBy);
        if (!result.success) {
            return result; // 如果任何一个失败，返回错误
        }
    }

    return Result<bool>::Success(true);
}

Result<bool> PermissionManager::batchGrantMenusToRole(int roleId, const std::vector<int>& menuIds) {
    if (!dbManager || !dbManager->isConnectionValid()) {
        return Result<bool>::Error("数据库未连接");
    }

    // 开始事务
    if (!dbManager->beginTransaction()) {
        return Result<bool>::Error("开始事务失败");
    }

    try {
        // 1. 先撤销该角色的所有菜单权限
        std::string revokeSql = "UPDATE role_menus SET is_granted = FALSE WHERE role_id = " + std::to_string(roleId) + ";";
        auto revokeResult = dbManager->executeQuery(revokeSql);
        if (!revokeResult.success) {
            dbManager->rollbackTransaction();
            return Result<bool>::Error("撤销现有权限失败: " + revokeResult.message);
        }

        // 2. 为选中的菜单授予权限
        for (int menuId : menuIds) {
            std::string grantSql = "INSERT INTO role_menus (role_id, menu_id, is_granted) VALUES (" +
                                  std::to_string(roleId) + ", " + std::to_string(menuId) + ", TRUE) "
                                  "ON DUPLICATE KEY UPDATE is_granted = TRUE;";
            
            auto grantResult = dbManager->executeQuery(grantSql);
            if (!grantResult.success) {
                dbManager->rollbackTransaction();
                return Result<bool>::Error("授予菜单权限失败: " + grantResult.message);
            }
        }

        // 3. 提交事务
        if (!dbManager->commitTransaction()) {
            dbManager->rollbackTransaction();
            return Result<bool>::Error("提交事务失败");
        }

        // 清除相关缓存
        clearCache();

        return Result<bool>::Success(true);
    } catch (const std::exception& e) {
        dbManager->rollbackTransaction();
        return Result<bool>::Error("批量授权菜单权限时发生异常: " + std::string(e.what()));
    }
}

// 静态辅助方法
bool PermissionManager::isValidPermissionKey(const std::string& permissionKey) {
    // 简单的权限键验证：格式应该是 module:action
    size_t colonPos = permissionKey.find(':');
    return colonPos != std::string::npos && colonPos > 0 && colonPos < permissionKey.length() - 1;
}

std::vector<std::string> PermissionManager::parsePermissionKey(const std::string& permissionKey) {
    std::vector<std::string> parts;
    size_t colonPos = permissionKey.find(':');
    if (colonPos != std::string::npos) {
        parts.push_back(permissionKey.substr(0, colonPos));
        parts.push_back(permissionKey.substr(colonPos + 1));
    }
    return parts;
}
