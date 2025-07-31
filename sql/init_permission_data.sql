-- 初始化权限数据脚本
-- 用于快速设置基础的角色和权限数据

-- 1. 插入基础角色
INSERT IGNORE INTO roles (role_name, role_code, description, is_system, is_active) VALUES
('超级管理员', 'SUPER_ADMIN', '系统超级管理员，拥有所有权限', TRUE, TRUE),
('管理员', 'ADMIN', '系统管理员，负责用户和权限管理', TRUE, TRUE),
('普通用户', 'USER', '普通用户，基础功能权限', TRUE, TRUE);

-- 2. 插入基础菜单结构
INSERT IGNORE INTO menus (name, code, parent_id, type, url, icon, permission_key, sort_order, description) VALUES
-- 一级菜单
('首页', 'HOME', NULL, 'MENU', '/home', 'home', 'home:view', 1, '系统首页'),
('文档管理', 'DOCUMENT_MANAGEMENT', NULL, 'DIRECTORY', NULL, 'folder', 'document:module', 2, '文档管理模块'),
('用户管理', 'USER_MANAGEMENT', NULL, 'DIRECTORY', NULL, 'users', 'user:module', 3, '用户管理模块'),
('权限管理', 'PERMISSION_MANAGEMENT', NULL, 'DIRECTORY', NULL, 'shield', 'permission:module', 4, '权限管理模块');

-- 获取父菜单ID
SET @doc_id = (SELECT id FROM menus WHERE code = 'DOCUMENT_MANAGEMENT');
SET @user_id = (SELECT id FROM menus WHERE code = 'USER_MANAGEMENT');
SET @perm_id = (SELECT id FROM menus WHERE code = 'PERMISSION_MANAGEMENT');

-- 文档管理子菜单
INSERT IGNORE INTO menus (name, code, parent_id, type, url, icon, permission_key, sort_order, description) VALUES
('我的文档', 'MY_DOCUMENTS', @doc_id, 'MENU', '/documents/my', 'file-text', 'document:my:view', 1, '我的文档列表'),
('分享文档', 'SHARED_DOCUMENTS', @doc_id, 'MENU', '/documents/shared', 'share', 'document:shared:view', 2, '分享给我的文档');

-- 我的文档按钮权限
SET @my_doc_id = (SELECT id FROM menus WHERE code = 'MY_DOCUMENTS');
INSERT IGNORE INTO menus (name, code, parent_id, type, permission_key, button_type, sort_order, description) VALUES
('上传文档', 'UPLOAD_DOC', @my_doc_id, 'BUTTON', 'document:upload', 'ADD', 1, '上传新文档'),
('编辑文档', 'EDIT_DOC', @my_doc_id, 'BUTTON', 'document:edit', 'EDIT', 2, '编辑文档信息'),
('删除文档', 'DELETE_DOC', @my_doc_id, 'BUTTON', 'document:delete', 'DELETE', 3, '删除文档'),
('下载文档', 'DOWNLOAD_DOC', @my_doc_id, 'BUTTON', 'document:download', 'VIEW', 4, '下载文档'),
('分享文档', 'SHARE_DOC', @my_doc_id, 'BUTTON', 'document:share', 'CUSTOM', 5, '分享文档给其他用户');

-- 分享文档按钮权限
SET @shared_doc_id = (SELECT id FROM menus WHERE code = 'SHARED_DOCUMENTS');
INSERT IGNORE INTO menus (name, code, parent_id, type, permission_key, button_type, sort_order, description) VALUES
('查看文档', 'VIEW_SHARED_DOC', @shared_doc_id, 'BUTTON', 'document:shared:view', 'VIEW', 1, '查看分享的文档'),
('下载分享文档', 'DOWNLOAD_SHARED_DOC', @shared_doc_id, 'BUTTON', 'document:shared:download', 'VIEW', 2, '下载分享的文档');

-- 用户管理子菜单
INSERT IGNORE INTO menus (name, code, parent_id, type, url, icon, permission_key, sort_order, description) VALUES
('用户列表', 'USER_LIST', @user_id, 'MENU', '/users/list', 'user', 'user:list', 1, '用户列表管理');

-- 用户列表按钮权限
SET @user_list_id = (SELECT id FROM menus WHERE code = 'USER_LIST');
INSERT IGNORE INTO menus (name, code, parent_id, type, permission_key, button_type, sort_order, description) VALUES
('添加用户', 'ADD_USER', @user_list_id, 'BUTTON', 'user:add', 'ADD', 1, '添加新用户'),
('编辑用户', 'EDIT_USER', @user_list_id, 'BUTTON', 'user:edit', 'EDIT', 2, '编辑用户信息'),
('删除用户', 'DELETE_USER', @user_list_id, 'BUTTON', 'user:delete', 'DELETE', 3, '删除用户'),
('重置密码', 'RESET_PASSWORD', @user_list_id, 'BUTTON', 'user:reset_password', 'CUSTOM', 4, '重置用户密码');

-- 权限管理子菜单
INSERT IGNORE INTO menus (name, code, parent_id, type, url, icon, permission_key, sort_order, description) VALUES
('角色管理', 'ROLE_MANAGEMENT', @perm_id, 'MENU', '/permissions/roles', 'users', 'role:manage', 1, '角色管理'),
('权限分配', 'PERMISSION_ASSIGNMENT', @perm_id, 'MENU', '/permissions/assignment', 'key', 2, '权限分配管理');

-- 角色管理按钮权限
SET @role_mgmt_id = (SELECT id FROM menus WHERE code = 'ROLE_MANAGEMENT');
INSERT IGNORE INTO menus (name, code, parent_id, type, permission_key, button_type, sort_order, description) VALUES
('添加角色', 'ADD_ROLE', @role_mgmt_id, 'BUTTON', 'role:add', 'ADD', 1, '添加新角色'),
('编辑角色', 'EDIT_ROLE', @role_mgmt_id, 'BUTTON', 'role:edit', 'EDIT', 2, '编辑角色信息'),
('删除角色', 'DELETE_ROLE', @role_mgmt_id, 'BUTTON', 'role:delete', 'DELETE', 3, '删除角色'),
('分配权限', 'ASSIGN_PERMISSION', @role_mgmt_id, 'BUTTON', 'role:assign_permission', 'CUSTOM', 4, '为角色分配权限');

-- 权限分配按钮权限
SET @perm_assign_id = (SELECT id FROM menus WHERE code = 'PERMISSION_ASSIGNMENT');
INSERT IGNORE INTO menus (name, code, parent_id, type, permission_key, button_type, sort_order, description) VALUES
('分配角色', 'ASSIGN_USER_ROLE', @perm_assign_id, 'BUTTON', 'permission:assign', 'CUSTOM', 1, '为用户分配角色'),
('管理权限', 'MANAGE_PERMISSION', @perm_assign_id, 'BUTTON', 'permission:manage', 'CUSTOM', 2, '管理系统权限');

-- 3. 为超级管理员分配所有权限
SET @super_admin_id = (SELECT id FROM roles WHERE role_code = 'SUPER_ADMIN');
INSERT IGNORE INTO role_menus (role_id, menu_id, is_granted)
SELECT @super_admin_id, id, TRUE FROM menus;

-- 4. 为管理员分配管理权限
SET @admin_id = (SELECT id FROM roles WHERE role_code = 'ADMIN');
INSERT IGNORE INTO role_menus (role_id, menu_id, is_granted)
SELECT @admin_id, id, TRUE FROM menus 
WHERE permission_key IN (
    'home:view', 'user:module', 'user:list', 'user:add', 'user:edit', 'user:delete', 'user:reset_password',
    'permission:module', 'role:manage', 'role:add', 'role:edit', 'role:delete', 'role:assign_permission',
    'permission:assign', 'permission:manage'
);

-- 5. 为普通用户分配基础权限
SET @user_id = (SELECT id FROM roles WHERE role_code = 'USER');
INSERT IGNORE INTO role_menus (role_id, menu_id, is_granted)
SELECT @user_id, id, TRUE FROM menus 
WHERE permission_key IN (
    'home:view', 'document:module', 'document:my:view', 'document:shared:view',
    'document:upload', 'document:edit', 'document:download', 'document:share',
    'document:shared:view', 'document:shared:download'
);

-- 6. 为第一个用户（通常是管理员）分配超级管理员角色
-- 注意：这里假设第一个用户ID为1，实际使用时可能需要调整
INSERT IGNORE INTO user_roles (user_id, role_id, assigned_by)
SELECT 1, @super_admin_id, 1
WHERE EXISTS (SELECT 1 FROM users WHERE id = 1);

-- 显示初始化结果
SELECT '角色初始化完成' as message;
SELECT role_name, role_code, description FROM roles;

SELECT '菜单初始化完成' as message;
SELECT name, code, type, permission_key FROM menus ORDER BY sort_order;

SELECT '权限分配完成' as message;
SELECT r.role_name, COUNT(rm.menu_id) as menu_count 
FROM roles r 
LEFT JOIN role_menus rm ON r.id = rm.role_id AND rm.is_granted = TRUE
GROUP BY r.id, r.role_name;
