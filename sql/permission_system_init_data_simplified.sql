-- 权限控制系统初始化数据（简化版）
-- 创建时间: 2025-07-21
-- 说明: 初始化基础角色、菜单和权限数据

-- 1. 插入系统角色
INSERT INTO roles (role_name, role_code, description, is_system, is_active) VALUES
('超级管理员', 'SUPER_ADMIN', '系统超级管理员，拥有所有权限', TRUE, TRUE),
('系统管理员', 'SYSTEM_ADMIN', '系统管理员，负责用户和权限管理', TRUE, TRUE),
('文档管理员', 'DOC_ADMIN', '文档管理员，负责文档管理相关功能', TRUE, TRUE),
('普通用户', 'NORMAL_USER', '普通用户，基础功能权限', TRUE, TRUE),
('访客用户', 'GUEST_USER', '访客用户，只读权限', TRUE, TRUE);

-- 2. 插入菜单结构（树形结构，包含菜单和按钮）
-- 一级菜单（目录）
INSERT INTO menus (name, code, parent_id, type, url, icon, sort_order, description, permission_key) VALUES
('首页', 'HOME', NULL, 'MENU', '/home', 'home', 1, '系统首页', 'home:view'),
('文档管理', 'DOCUMENT_MANAGEMENT', NULL, 'DIRECTORY', NULL, 'folder', 2, '文档管理模块', 'document:module'),
('用户管理', 'USER_MANAGEMENT', NULL, 'DIRECTORY', NULL, 'users', 3, '用户管理模块', 'user:module'),
('系统管理', 'SYSTEM_MANAGEMENT', NULL, 'DIRECTORY', NULL, 'settings', 4, '系统管理模块', 'system:module'),
('权限管理', 'PERMISSION_MANAGEMENT', NULL, 'DIRECTORY', NULL, 'shield', 5, '权限管理模块', 'permission:module');

-- 文档管理子菜单
INSERT INTO menus (name, code, parent_id, type, url, icon, sort_order, description, permission_key) VALUES
('我的文档', 'MY_DOCUMENTS', (SELECT id FROM menus WHERE code = 'DOCUMENT_MANAGEMENT'), 'MENU', '/documents/my', 'file-text', 1, '我的文档列表', 'document:my:view'),
('分享给我的文档', 'SHARED_DOCUMENTS', (SELECT id FROM menus WHERE code = 'DOCUMENT_MANAGEMENT'), 'MENU', '/documents/shared', 'share', 2, '分享给我的文档', 'document:shared:view'),
('文档搜索', 'DOCUMENT_SEARCH', (SELECT id FROM menus WHERE code = 'DOCUMENT_MANAGEMENT'), 'MENU', '/documents/search', 'search', 3, '文档搜索功能', 'document:search'),
('文档统计', 'DOCUMENT_STATISTICS', (SELECT id FROM menus WHERE code = 'DOCUMENT_MANAGEMENT'), 'MENU', '/documents/statistics', 'bar-chart', 4, '文档统计报表', 'document:statistics');

-- 我的文档页面按钮
INSERT INTO menus (name, code, parent_id, type, url, icon, sort_order, description, permission_key, button_type) VALUES
('上传文档', 'UPLOAD_DOC', (SELECT id FROM menus WHERE code = 'MY_DOCUMENTS'), 'BUTTON', NULL, 'upload', 1, '上传新文档', 'document:upload', 'ADD'),
('编辑文档', 'EDIT_DOC', (SELECT id FROM menus WHERE code = 'MY_DOCUMENTS'), 'BUTTON', NULL, 'edit', 2, '编辑文档信息', 'document:edit', 'EDIT'),
('删除文档', 'DELETE_DOC', (SELECT id FROM menus WHERE code = 'MY_DOCUMENTS'), 'BUTTON', NULL, 'delete', 3, '删除文档', 'document:delete', 'DELETE'),
('下载文档', 'DOWNLOAD_DOC', (SELECT id FROM menus WHERE code = 'MY_DOCUMENTS'), 'BUTTON', NULL, 'download', 4, '下载文档', 'document:download', 'VIEW'),
('分享文档', 'SHARE_DOC', (SELECT id FROM menus WHERE code = 'MY_DOCUMENTS'), 'BUTTON', NULL, 'share', 5, '分享文档给其他用户', 'document:share', 'CUSTOM'),
('导出列表', 'EXPORT_DOC_LIST', (SELECT id FROM menus WHERE code = 'MY_DOCUMENTS'), 'BUTTON', NULL, 'export', 6, '导出文档列表', 'document:export', 'EXPORT');

-- 分享文档页面按钮
INSERT INTO menus (name, code, parent_id, type, url, icon, sort_order, description, permission_key, button_type) VALUES
('查看文档', 'VIEW_SHARED_DOC', (SELECT id FROM menus WHERE code = 'SHARED_DOCUMENTS'), 'BUTTON', NULL, 'eye', 1, '查看分享的文档', 'document:shared:view', 'VIEW'),
('下载文档', 'DOWNLOAD_SHARED_DOC', (SELECT id FROM menus WHERE code = 'SHARED_DOCUMENTS'), 'BUTTON', NULL, 'download', 2, '下载分享的文档', 'document:shared:download', 'VIEW');

-- 用户管理子菜单
INSERT INTO menus (name, code, parent_id, type, url, icon, sort_order, description, permission_key) VALUES
('用户列表', 'USER_LIST', (SELECT id FROM menus WHERE code = 'USER_MANAGEMENT'), 'MENU', '/users/list', 'user', 1, '用户列表管理', 'user:list'),
('用户搜索', 'USER_SEARCH', (SELECT id FROM menus WHERE code = 'USER_MANAGEMENT'), 'MENU', '/users/search', 'search', 2, '用户搜索功能', 'user:search');

-- 用户列表页面按钮
INSERT INTO menus (name, code, parent_id, type, url, icon, sort_order, description, permission_key, button_type) VALUES
('添加用户', 'ADD_USER', (SELECT id FROM menus WHERE code = 'USER_LIST'), 'BUTTON', NULL, 'user-plus', 1, '添加新用户', 'user:add', 'ADD'),
('编辑用户', 'EDIT_USER', (SELECT id FROM menus WHERE code = 'USER_LIST'), 'BUTTON', NULL, 'user-edit', 2, '编辑用户信息', 'user:edit', 'EDIT'),
('删除用户', 'DELETE_USER', (SELECT id FROM menus WHERE code = 'USER_LIST'), 'BUTTON', NULL, 'user-delete', 3, '删除用户', 'user:delete', 'DELETE'),
('重置密码', 'RESET_PASSWORD', (SELECT id FROM menus WHERE code = 'USER_LIST'), 'BUTTON', NULL, 'key', 4, '重置用户密码', 'user:reset_password', 'CUSTOM'),
('分配角色', 'ASSIGN_ROLE', (SELECT id FROM menus WHERE code = 'USER_LIST'), 'BUTTON', NULL, 'user-check', 5, '为用户分配角色', 'user:assign_role', 'CUSTOM'),
('导出用户', 'EXPORT_USER', (SELECT id FROM menus WHERE code = 'USER_LIST'), 'BUTTON', NULL, 'download', 6, '导出用户列表', 'user:export', 'EXPORT');

-- 权限管理子菜单
INSERT INTO menus (name, code, parent_id, type, url, icon, sort_order, description, permission_key) VALUES
('角色管理', 'ROLE_MANAGEMENT', (SELECT id FROM menus WHERE code = 'PERMISSION_MANAGEMENT'), 'MENU', '/permissions/roles', 'users', 1, '角色管理', 'role:manage'),
('菜单管理', 'MENU_MANAGEMENT', (SELECT id FROM menus WHERE code = 'PERMISSION_MANAGEMENT'), 'MENU', '/permissions/menus', 'menu', 2, '菜单管理', 'menu:manage'),
('权限分配', 'PERMISSION_ASSIGNMENT', (SELECT id FROM menus WHERE code = 'PERMISSION_MANAGEMENT'), 'MENU', '/permissions/assignment', 'key', 3, '权限分配管理', 'permission:assign');

-- 角色管理页面按钮
INSERT INTO menus (name, code, parent_id, type, url, icon, sort_order, description, permission_key, button_type) VALUES
('添加角色', 'ADD_ROLE', (SELECT id FROM menus WHERE code = 'ROLE_MANAGEMENT'), 'BUTTON', NULL, 'plus', 1, '添加新角色', 'role:add', 'ADD'),
('编辑角色', 'EDIT_ROLE', (SELECT id FROM menus WHERE code = 'ROLE_MANAGEMENT'), 'BUTTON', NULL, 'edit', 2, '编辑角色信息', 'role:edit', 'EDIT'),
('删除角色', 'DELETE_ROLE', (SELECT id FROM menus WHERE code = 'ROLE_MANAGEMENT'), 'BUTTON', NULL, 'delete', 3, '删除角色', 'role:delete', 'DELETE'),
('分配权限', 'ASSIGN_PERMISSION', (SELECT id FROM menus WHERE code = 'ROLE_MANAGEMENT'), 'BUTTON', NULL, 'key', 4, '为角色分配权限', 'role:assign_permission', 'CUSTOM');

-- 菜单管理页面按钮
INSERT INTO menus (name, code, parent_id, type, url, icon, sort_order, description, permission_key, button_type) VALUES
('添加菜单', 'ADD_MENU', (SELECT id FROM menus WHERE code = 'MENU_MANAGEMENT'), 'BUTTON', NULL, 'plus', 1, '添加新菜单', 'menu:add', 'ADD'),
('编辑菜单', 'EDIT_MENU', (SELECT id FROM menus WHERE code = 'MENU_MANAGEMENT'), 'BUTTON', NULL, 'edit', 2, '编辑菜单信息', 'menu:edit', 'EDIT'),
('删除菜单', 'DELETE_MENU', (SELECT id FROM menus WHERE code = 'MENU_MANAGEMENT'), 'BUTTON', NULL, 'delete', 3, '删除菜单', 'menu:delete', 'DELETE'),
('添加按钮', 'ADD_MENU_BUTTON', (SELECT id FROM menus WHERE code = 'MENU_MANAGEMENT'), 'BUTTON', NULL, 'plus-circle', 4, '为菜单添加按钮', 'menu:add_button', 'ADD');

-- 系统管理子菜单
INSERT INTO menus (name, code, parent_id, type, url, icon, sort_order, description, permission_key) VALUES
('系统配置', 'SYSTEM_CONFIG', (SELECT id FROM menus WHERE code = 'SYSTEM_MANAGEMENT'), 'MENU', '/system/config', 'settings', 1, '系统配置管理', 'system:config'),
('操作日志', 'OPERATION_LOG', (SELECT id FROM menus WHERE code = 'SYSTEM_MANAGEMENT'), 'MENU', '/system/logs', 'file-text', 2, '系统操作日志', 'system:log'),
('数据备份', 'DATA_BACKUP', (SELECT id FROM menus WHERE code = 'SYSTEM_MANAGEMENT'), 'MENU', '/system/backup', 'database', 3, '数据备份管理', 'system:backup');

-- 3. 为超级管理员分配所有权限
INSERT INTO role_menus (role_id, menu_id, is_granted)
SELECT 
    (SELECT id FROM roles WHERE role_code = 'SUPER_ADMIN'),
    id,
    TRUE
FROM menus;

-- 4. 为普通用户分配基础权限（文档相关）
INSERT INTO role_menus (role_id, menu_id, is_granted)
SELECT 
    (SELECT id FROM roles WHERE role_code = 'NORMAL_USER'),
    id,
    TRUE
FROM menus
WHERE code IN (
    'HOME', 'DOCUMENT_MANAGEMENT', 'MY_DOCUMENTS', 'SHARED_DOCUMENTS', 'DOCUMENT_SEARCH',
    'UPLOAD_DOC', 'EDIT_DOC', 'DOWNLOAD_DOC', 'SHARE_DOC', 'EXPORT_DOC_LIST',
    'VIEW_SHARED_DOC', 'DOWNLOAD_SHARED_DOC'
);

-- 5. 为访客用户分配只读权限
INSERT INTO role_menus (role_id, menu_id, is_granted)
SELECT 
    (SELECT id FROM roles WHERE role_code = 'GUEST_USER'),
    id,
    TRUE
FROM menus
WHERE code IN (
    'HOME', 'SHARED_DOCUMENTS', 'DOCUMENT_SEARCH',
    'VIEW_SHARED_DOC', 'DOWNLOAD_SHARED_DOC'
);

-- 6. 为文档管理员分配文档相关的所有权限
INSERT INTO role_menus (role_id, menu_id, is_granted)
SELECT 
    (SELECT id FROM roles WHERE role_code = 'DOC_ADMIN'),
    id,
    TRUE
FROM menus
WHERE code LIKE 'DOCUMENT%' OR code LIKE '%DOC%' OR code = 'HOME';

-- 7. 为系统管理员分配用户和权限管理权限
INSERT INTO role_menus (role_id, menu_id, is_granted)
SELECT 
    (SELECT id FROM roles WHERE role_code = 'SYSTEM_ADMIN'),
    id,
    TRUE
FROM menus
WHERE code IN (
    'HOME', 'USER_MANAGEMENT', 'USER_LIST', 'USER_SEARCH',
    'ADD_USER', 'EDIT_USER', 'DELETE_USER', 'RESET_PASSWORD', 'ASSIGN_ROLE', 'EXPORT_USER',
    'PERMISSION_MANAGEMENT', 'ROLE_MANAGEMENT', 'MENU_MANAGEMENT', 'PERMISSION_ASSIGNMENT',
    'ADD_ROLE', 'EDIT_ROLE', 'DELETE_ROLE', 'ASSIGN_PERMISSION',
    'ADD_MENU', 'EDIT_MENU', 'DELETE_MENU', 'ADD_MENU_BUTTON',
    'SYSTEM_MANAGEMENT', 'SYSTEM_CONFIG', 'OPERATION_LOG', 'DATA_BACKUP'
);
