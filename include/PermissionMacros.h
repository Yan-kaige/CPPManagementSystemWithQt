#pragma once

#include "Common.h"
#include <QMessageBox>
#include <QWidget>

// 权限检查宏定义
// 用于简化代码中的权限检查

// 前向声明
class PermissionManager;

// 全局权限管理器指针（需要在主程序中初始化）
extern PermissionManager* g_permissionManager;

// 获取当前用户ID的函数（需要在主程序中实现）
extern int getCurrentUserId();

/**
 * 检查用户是否有指定权限
 * @param permission_key 权限标识
 * @return true表示有权限，false表示无权限
 */
#define HAS_PERMISSION(permission_key) \
    (g_permissionManager && g_permissionManager->hasPermission(getCurrentUserId(), permission_key).success && \
     g_permissionManager->hasPermission(getCurrentUserId(), permission_key).data.value_or(false))

/**
 * 检查用户是否有菜单访问权限
 * @param menu_code 菜单编码
 * @return true表示有权限，false表示无权限
 */
#define HAS_MENU_ACCESS(menu_code) \
    (g_permissionManager && g_permissionManager->hasMenuAccess(getCurrentUserId(), menu_code).success && \
     g_permissionManager->hasMenuAccess(getCurrentUserId(), menu_code).data.value_or(false))

/**
 * 检查用户是否有按钮权限
 * @param button_code 按钮编码
 * @return true表示有权限，false表示无权限
 */
#define HAS_BUTTON_ACCESS(button_code) \
    (g_permissionManager && g_permissionManager->hasButtonAccess(getCurrentUserId(), button_code).success && \
     g_permissionManager->hasButtonAccess(getCurrentUserId(), button_code).data.value_or(false))

/**
 * 权限检查，如果没有权限则显示错误消息并返回
 * @param permission_key 权限标识
 * @param parent_widget 父窗口指针
 * @param error_message 错误消息（可选）
 */
#define CHECK_PERMISSION_OR_RETURN(permission_key, parent_widget, error_message) \
    do { \
        if (!HAS_PERMISSION(permission_key)) { \
            QString msg = error_message; \
            if (msg.isEmpty()) { \
                msg = QString("您没有执行此操作的权限：%1").arg(permission_key); \
            } \
            QMessageBox::warning(parent_widget, "权限不足", msg); \
            return; \
        } \
    } while(0)

/**
 * 权限检查，如果没有权限则显示错误消息并返回指定值
 * @param permission_key 权限标识
 * @param parent_widget 父窗口指针
 * @param return_value 返回值
 * @param error_message 错误消息（可选）
 */
#define CHECK_PERMISSION_OR_RETURN_VALUE(permission_key, parent_widget, return_value, error_message) \
    do { \
        if (!HAS_PERMISSION(permission_key)) { \
            QString msg = error_message; \
            if (msg.isEmpty()) { \
                msg = QString("您没有执行此操作的权限：%1").arg(permission_key); \
            } \
            QMessageBox::warning(parent_widget, "权限不足", msg); \
            return return_value; \
        } \
    } while(0)

/**
 * 菜单权限检查，如果没有权限则显示错误消息并返回
 * @param menu_code 菜单编码
 * @param parent_widget 父窗口指针
 * @param error_message 错误消息（可选）
 */
#define CHECK_MENU_ACCESS_OR_RETURN(menu_code, parent_widget, error_message) \
    do { \
        if (!HAS_MENU_ACCESS(menu_code)) { \
            QString msg = error_message; \
            if (msg.isEmpty()) { \
                msg = QString("您没有访问此菜单的权限：%1").arg(menu_code); \
            } \
            QMessageBox::warning(parent_widget, "权限不足", msg); \
            return; \
        } \
    } while(0)

/**
 * 按钮权限检查，如果没有权限则显示错误消息并返回
 * @param button_code 按钮编码
 * @param parent_widget 父窗口指针
 * @param error_message 错误消息（可选）
 */
#define CHECK_BUTTON_ACCESS_OR_RETURN(button_code, parent_widget, error_message) \
    do { \
        if (!HAS_BUTTON_ACCESS(button_code)) { \
            QString msg = error_message; \
            if (msg.isEmpty()) { \
                msg = QString("您没有执行此操作的权限：%1").arg(button_code); \
            } \
            QMessageBox::warning(parent_widget, "权限不足", msg); \
            return; \
        } \
    } while(0)

/**
 * 设置控件的可见性（基于权限）
 * @param widget 控件指针
 * @param permission_key 权限标识
 */
#define SET_WIDGET_VISIBLE_BY_PERMISSION(widget, permission_key) \
    do { \
        if (widget) { \
            widget->setVisible(HAS_PERMISSION(permission_key)); \
        } \
    } while(0)

/**
 * 设置控件的启用状态（基于权限）
 * @param widget 控件指针
 * @param permission_key 权限标识
 */
#define SET_WIDGET_ENABLED_BY_PERMISSION(widget, permission_key) \
    do { \
        if (widget) { \
            widget->setEnabled(HAS_PERMISSION(permission_key)); \
        } \
    } while(0)

/**
 * 设置菜单项的可见性（基于菜单权限）
 * @param menu_item 菜单项指针
 * @param menu_code 菜单编码
 */
#define SET_MENU_VISIBLE_BY_ACCESS(menu_item, menu_code) \
    do { \
        if (menu_item) { \
            menu_item->setVisible(HAS_MENU_ACCESS(menu_code)); \
        } \
    } while(0)

/**
 * 设置按钮的可见性（基于按钮权限）
 * @param button 按钮指针
 * @param button_code 按钮编码
 */
#define SET_BUTTON_VISIBLE_BY_ACCESS(button, button_code) \
    do { \
        if (button) { \
            button->setVisible(HAS_BUTTON_ACCESS(button_code)); \
        } \
    } while(0)

// 常用权限标识常量
namespace PermissionKeys {
    // 首页权限
    constexpr const char* HOME_VIEW = "home:view";
    
    // 文档管理权限
    constexpr const char* DOCUMENT_MODULE = "document:module";
    constexpr const char* DOCUMENT_MY_VIEW = "document:my:view";
    constexpr const char* DOCUMENT_SHARED_VIEW = "document:shared:view";
    constexpr const char* DOCUMENT_SEARCH = "document:search";
    constexpr const char* DOCUMENT_STATISTICS = "document:statistics";
    constexpr const char* DOCUMENT_UPLOAD = "document:upload";
    constexpr const char* DOCUMENT_EDIT = "document:edit";
    constexpr const char* DOCUMENT_DELETE = "document:delete";
    constexpr const char* DOCUMENT_DOWNLOAD = "document:download";
    constexpr const char* DOCUMENT_SHARE = "document:share";
    constexpr const char* DOCUMENT_EXPORT = "document:export";
    
    // 用户管理权限
    constexpr const char* USER_MODULE = "user:module";
    constexpr const char* USER_LIST = "user:list";
    constexpr const char* USER_SEARCH = "user:search";
    constexpr const char* USER_ADD = "user:add";
    constexpr const char* USER_EDIT = "user:edit";
    constexpr const char* USER_DELETE = "user:delete";
    constexpr const char* USER_RESET_PASSWORD = "user:reset_password";
    constexpr const char* USER_ASSIGN_ROLE = "user:assign_role";
    constexpr const char* USER_EXPORT = "user:export";
    
    // 权限管理权限
    constexpr const char* PERMISSION_MODULE = "permission:module";
    constexpr const char* ROLE_MANAGE = "role:manage";
    constexpr const char* ROLE_ADD = "role:add";
    constexpr const char* ROLE_EDIT = "role:edit";
    constexpr const char* ROLE_DELETE = "role:delete";
    constexpr const char* ROLE_ASSIGN_PERMISSION = "role:assign_permission";
    constexpr const char* MENU_MANAGE = "menu:manage";
    constexpr const char* MENU_ADD = "menu:add";
    constexpr const char* MENU_EDIT = "menu:edit";
    constexpr const char* MENU_DELETE = "menu:delete";
    constexpr const char* MENU_ADD_BUTTON = "menu:add_button";
    constexpr const char* PERMISSION_ASSIGN = "permission:assign";
    
    // 系统管理权限
    constexpr const char* SYSTEM_MODULE = "system:module";
    constexpr const char* SYSTEM_CONFIG = "system:config";
    constexpr const char* SYSTEM_LOG = "system:log";
    constexpr const char* SYSTEM_BACKUP = "system:backup";
}

// 常用菜单编码常量
namespace MenuCodes {
    constexpr const char* HOME = "HOME";
    constexpr const char* DOCUMENT_MANAGEMENT = "DOCUMENT_MANAGEMENT";
    constexpr const char* MY_DOCUMENTS = "MY_DOCUMENTS";
    constexpr const char* SHARED_DOCUMENTS = "SHARED_DOCUMENTS";
    constexpr const char* USER_MANAGEMENT = "USER_MANAGEMENT";
    constexpr const char* USER_LIST = "USER_LIST";
    constexpr const char* PERMISSION_MANAGEMENT = "PERMISSION_MANAGEMENT";
    constexpr const char* ROLE_MANAGEMENT = "ROLE_MANAGEMENT";
    constexpr const char* MENU_MANAGEMENT = "MENU_MANAGEMENT";
    constexpr const char* SYSTEM_MANAGEMENT = "SYSTEM_MANAGEMENT";
}

// 常用按钮编码常量
namespace ButtonCodes {
    constexpr const char* UPLOAD_DOC = "UPLOAD_DOC";
    constexpr const char* EDIT_DOC = "EDIT_DOC";
    constexpr const char* DELETE_DOC = "DELETE_DOC";
    constexpr const char* DOWNLOAD_DOC = "DOWNLOAD_DOC";
    constexpr const char* SHARE_DOC = "SHARE_DOC";
    constexpr const char* ADD_USER = "ADD_USER";
    constexpr const char* EDIT_USER = "EDIT_USER";
    constexpr const char* DELETE_USER = "DELETE_USER";
    constexpr const char* RESET_PASSWORD = "RESET_PASSWORD";
    constexpr const char* ASSIGN_ROLE = "ASSIGN_ROLE";
}
