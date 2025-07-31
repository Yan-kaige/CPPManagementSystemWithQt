-- 权限控制系统数据库表结构（简化版）
-- 创建时间: 2025-07-21
-- 说明: 基于角色的权限控制系统(RBAC)，菜单和按钮统一管理

-- 1. 角色表
CREATE TABLE IF NOT EXISTS roles (
    id INT AUTO_INCREMENT PRIMARY KEY,
    role_name VARCHAR(100) NOT NULL UNIQUE COMMENT '角色名称',
    role_code VARCHAR(50) NOT NULL UNIQUE COMMENT '角色编码',
    description TEXT COMMENT '角色描述',
    is_active BOOLEAN DEFAULT TRUE COMMENT '是否启用',
    is_system BOOLEAN DEFAULT FALSE COMMENT '是否系统内置角色',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    created_by INT COMMENT '创建人ID',
    updated_by INT COMMENT '更新人ID',
    INDEX idx_role_code (role_code),
    INDEX idx_is_active (is_active)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='角色表';

-- 2. 菜单表（包含菜单和按钮的树形结构）
CREATE TABLE IF NOT EXISTS menus (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) NOT NULL COMMENT '名称（菜单名或按钮名）',
    code VARCHAR(50) NOT NULL UNIQUE COMMENT '编码（菜单编码或按钮编码）',
    parent_id INT DEFAULT NULL COMMENT '父级ID，NULL表示根菜单',
    type ENUM('DIRECTORY', 'MENU', 'BUTTON') DEFAULT 'MENU' COMMENT '类型：DIRECTORY-目录，MENU-菜单页面，BUTTON-按钮权限',
    url VARCHAR(200) COMMENT '菜单URL或路由（按钮类型可为空）',
    icon VARCHAR(100) COMMENT '图标',
    permission_key VARCHAR(100) COMMENT '权限标识（用于代码中权限判断）',
    button_type ENUM('ADD', 'EDIT', 'DELETE', 'VIEW', 'EXPORT', 'IMPORT', 'CUSTOM') COMMENT '按钮类型（仅当type=BUTTON时有效）',
    sort_order INT DEFAULT 0 COMMENT '排序号',
    is_visible BOOLEAN DEFAULT TRUE COMMENT '是否可见（影响菜单显示）',
    is_active BOOLEAN DEFAULT TRUE COMMENT '是否启用',
    description TEXT COMMENT '描述',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    created_by INT COMMENT '创建人ID',
    updated_by INT COMMENT '更新人ID',
    INDEX idx_parent_id (parent_id),
    INDEX idx_code (code),
    INDEX idx_type (type),
    INDEX idx_sort_order (sort_order),
    INDEX idx_is_active (is_active),
    INDEX idx_permission_key (permission_key),
    FOREIGN KEY (parent_id) REFERENCES menus(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='菜单表（树形结构，包含菜单和按钮）';

-- 3. 用户角色关联表 (多对多)
CREATE TABLE IF NOT EXISTS user_roles (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL COMMENT '用户ID',
    role_id INT NOT NULL COMMENT '角色ID',
    assigned_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '分配时间',
    assigned_by INT COMMENT '分配人ID',
    is_active BOOLEAN DEFAULT TRUE COMMENT '是否启用',
    expires_at TIMESTAMP NULL COMMENT '过期时间，NULL表示永不过期',
    INDEX idx_user_id (user_id),
    INDEX idx_role_id (role_id),
    INDEX idx_is_active (is_active),
    INDEX idx_expires_at (expires_at),
    UNIQUE KEY uk_user_role (user_id, role_id),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (role_id) REFERENCES roles(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='用户角色关联表';

-- 4. 角色菜单关联表 (多对多，统一管理菜单和按钮权限)
CREATE TABLE IF NOT EXISTS role_menus (
    id INT AUTO_INCREMENT PRIMARY KEY,
    role_id INT NOT NULL COMMENT '角色ID',
    menu_id INT NOT NULL COMMENT '菜单ID（可以是目录、菜单页面或按钮）',
    is_granted BOOLEAN DEFAULT TRUE COMMENT '是否授权',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    created_by INT COMMENT '创建人ID',
    INDEX idx_role_id (role_id),
    INDEX idx_menu_id (menu_id),
    INDEX idx_is_granted (is_granted),
    UNIQUE KEY uk_role_menu (role_id, menu_id),
    FOREIGN KEY (role_id) REFERENCES roles(id) ON DELETE CASCADE,
    FOREIGN KEY (menu_id) REFERENCES menus(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='角色菜单关联表（统一管理菜单和按钮权限）';

-- 5. 权限日志表 (可选，用于审计)
CREATE TABLE IF NOT EXISTS permission_logs (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL COMMENT '用户ID',
    action_type ENUM('LOGIN', 'LOGOUT', 'ACCESS_MENU', 'ACCESS_BUTTON', 'PERMISSION_CHANGE') NOT NULL COMMENT '操作类型',
    resource_type ENUM('MENU', 'BUTTON', 'ROLE', 'USER') COMMENT '资源类型',
    resource_id INT COMMENT '资源ID',
    resource_name VARCHAR(200) COMMENT '资源名称',
    operation_result ENUM('SUCCESS', 'DENIED', 'ERROR') DEFAULT 'SUCCESS' COMMENT '操作结果',
    ip_address VARCHAR(45) COMMENT 'IP地址',
    user_agent TEXT COMMENT '用户代理',
    description TEXT COMMENT '操作描述',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    INDEX idx_user_id (user_id),
    INDEX idx_action_type (action_type),
    INDEX idx_created_at (created_at),
    INDEX idx_operation_result (operation_result),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='权限操作日志表';

-- 6. 创建一些常用的查询视图
-- 用户权限视图（展示用户拥有的所有权限）
CREATE VIEW user_permissions AS
SELECT 
    u.id as user_id,
    u.username,
    r.id as role_id,
    r.role_name,
    r.role_code,
    m.id as menu_id,
    m.name as menu_name,
    m.code as menu_code,
    m.type as menu_type,
    m.url,
    m.permission_key,
    rm.is_granted,
    ur.is_active as role_active,
    ur.expires_at
FROM users u
JOIN user_roles ur ON u.id = ur.user_id
JOIN roles r ON ur.role_id = r.id
JOIN role_menus rm ON r.id = rm.role_id
JOIN menus m ON rm.menu_id = m.id
WHERE u.is_active = TRUE 
    AND r.is_active = TRUE 
    AND ur.is_active = TRUE
    AND m.is_active = TRUE
    AND rm.is_granted = TRUE
    AND (ur.expires_at IS NULL OR ur.expires_at > NOW());

-- 菜单树形结构视图
CREATE VIEW menu_tree AS
WITH RECURSIVE menu_hierarchy AS (
    -- 根节点
    SELECT 
        id, name, code, parent_id, type, url, icon, 
        permission_key, button_type, sort_order, 
        is_visible, is_active, description,
        0 as level,
        CAST(code AS CHAR(1000)) as path
    FROM menus 
    WHERE parent_id IS NULL AND is_active = TRUE
    
    UNION ALL
    
    -- 递归查找子节点
    SELECT 
        m.id, m.name, m.code, m.parent_id, m.type, m.url, m.icon,
        m.permission_key, m.button_type, m.sort_order,
        m.is_visible, m.is_active, m.description,
        mh.level + 1,
        CONCAT(mh.path, '/', m.code)
    FROM menus m
    JOIN menu_hierarchy mh ON m.parent_id = mh.id
    WHERE m.is_active = TRUE
)
SELECT * FROM menu_hierarchy
ORDER BY level, sort_order, name;
