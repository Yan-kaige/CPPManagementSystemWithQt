#include "DatabaseManager.h"
#include "Common.h"
#include "Logger.h"
#include <QFileInfo>
#include <QDir>
#include <mutex>
#include <sstream>
DatabaseManager::DatabaseManager() : db(nullptr), isConnected(false) {

}

DatabaseManager::~DatabaseManager() {
    disconnect();
}

bool DatabaseManager::connect(const std::string& host, int port, const std::string& username, 
                             const std::string& password, const std::string& database) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);

    try {
        // 初始化MySQL连接
        db = mysql_init(nullptr);
        if (!db) {
            LOG_ERROR("无法初始化MySQL连接");
            return false;
        }

        // 设置连接选项
        mysql_options(db, MYSQL_OPT_RECONNECT, "1");
        mysql_options(db, MYSQL_SET_CHARSET_NAME, "utf8mb4");
        mysql_options(db, MYSQL_DEFAULT_AUTH, "mysql_native_password");

        //打印连接信息
        LOG_INFO("尝试连接到MySQL数据库: " + host + ":" + std::to_string(port) + "/" + database+
                 " 用户: " + username+" 密码: " + (password.empty() ? "未设置" : "已设置"));

        // 连接到MySQL服务器
        if (!mysql_real_connect(db, host.c_str(), username.c_str(), password.c_str(), 
                               database.c_str(), port, nullptr, 0)) {
            LOG_ERROR("无法连接到MySQL数据库: " + std::string(mysql_error(db)));
            mysql_close(db);
            db = nullptr;
            return false;
        }

        isConnected = true;
        LOG_INFO("MySQL数据库连接成功: " + host + ":" + std::to_string(port) + "/" + database);

        // 创建表
        if (!createTables()) {
            LOG_ERROR("创建数据库表失败");
            disconnect();
            return false;
        }

        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("数据库连接异常: " + std::string(e.what()));
        return false;
    }
}

void DatabaseManager::disconnect() {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);

    if (db) {
        mysql_close(db);
        db = nullptr;
        isConnected = false;
        LOG_INFO("MySQL数据库连接已关闭");
    }
}

bool DatabaseManager::isConnectionValid() const {
    return isConnected && db != nullptr;
}

bool DatabaseManager::createTables() {
    std::string createUsersTable = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INT AUTO_INCREMENT PRIMARY KEY,
            username VARCHAR(255) UNIQUE NOT NULL,
            password_hash VARCHAR(255) NOT NULL,
            email VARCHAR(255) UNIQUE NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            last_login TIMESTAMP NULL,
            is_active BOOLEAN DEFAULT TRUE
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
    )";

    std::string createDocumentsTable = R"(
        CREATE TABLE IF NOT EXISTS documents (
            id INT AUTO_INCREMENT PRIMARY KEY,
            title VARCHAR(255) NOT NULL,
            description TEXT,
            file_path VARCHAR(500),
            minio_key VARCHAR(500),
            owner_id INT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
            file_size BIGINT DEFAULT 0,
            content_type VARCHAR(100) DEFAULT '',
            FOREIGN KEY (owner_id) REFERENCES users(id) ON DELETE CASCADE
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
    )";

    std::string createDocumentSharesTable = R"(
        CREATE TABLE IF NOT EXISTS document_shares (
            id INT AUTO_INCREMENT PRIMARY KEY,
            document_id INT NOT NULL,
            shared_by_user_id INT NOT NULL,
            shared_to_user_id INT NOT NULL,
            shared_document_id INT NOT NULL,
            shared_minio_key VARCHAR(500),
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (document_id) REFERENCES documents(id) ON DELETE CASCADE,
            FOREIGN KEY (shared_by_user_id) REFERENCES users(id) ON DELETE CASCADE,
            FOREIGN KEY (shared_to_user_id) REFERENCES users(id) ON DELETE CASCADE,
            FOREIGN KEY (shared_document_id) REFERENCES documents(id) ON DELETE CASCADE,
            UNIQUE KEY unique_share (document_id, shared_by_user_id, shared_to_user_id)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
    )";

    // 权限管理相关表
    std::string createRolesTable = R"(
        CREATE TABLE IF NOT EXISTS roles (
            id INT AUTO_INCREMENT PRIMARY KEY,
            role_name VARCHAR(100) NOT NULL UNIQUE,
            role_code VARCHAR(50) NOT NULL UNIQUE,
            description TEXT,
            is_active BOOLEAN DEFAULT TRUE,
            is_system BOOLEAN DEFAULT FALSE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
            created_by INT,
            updated_by INT,
            INDEX idx_role_code (role_code),
            INDEX idx_is_active (is_active)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
    )";

    std::string createMenusTable = R"(
        CREATE TABLE IF NOT EXISTS menus (
            id INT AUTO_INCREMENT PRIMARY KEY,
            name VARCHAR(100) NOT NULL,
            code VARCHAR(50) NOT NULL UNIQUE,
            parent_id INT DEFAULT NULL,
            type ENUM('DIRECTORY', 'MENU', 'BUTTON') DEFAULT 'MENU',
            url VARCHAR(200),
            icon VARCHAR(100),
            permission_key VARCHAR(100),
            button_type ENUM('ADD', 'EDIT', 'DELETE', 'VIEW', 'EXPORT', 'IMPORT', 'CUSTOM'),
            sort_order INT DEFAULT 0,
            is_visible BOOLEAN DEFAULT TRUE,
            is_active BOOLEAN DEFAULT TRUE,
            description TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
            created_by INT,
            updated_by INT,
            INDEX idx_parent_id (parent_id),
            INDEX idx_code (code),
            INDEX idx_type (type),
            INDEX idx_sort_order (sort_order),
            INDEX idx_is_active (is_active),
            INDEX idx_permission_key (permission_key),
            FOREIGN KEY (parent_id) REFERENCES menus(id) ON DELETE CASCADE
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
    )";

    std::string createUserRolesTable = R"(
        CREATE TABLE IF NOT EXISTS user_roles (
            id INT AUTO_INCREMENT PRIMARY KEY,
            user_id INT NOT NULL,
            role_id INT NOT NULL,
            assigned_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            assigned_by INT,
            is_active BOOLEAN DEFAULT TRUE,
            expires_at TIMESTAMP NULL,
            INDEX idx_user_id (user_id),
            INDEX idx_role_id (role_id),
            INDEX idx_is_active (is_active),
            INDEX idx_expires_at (expires_at),
            UNIQUE KEY uk_user_role (user_id, role_id),
            FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
            FOREIGN KEY (role_id) REFERENCES roles(id) ON DELETE CASCADE
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
    )";

    std::string createRoleMenusTable = R"(
        CREATE TABLE IF NOT EXISTS role_menus (
            id INT AUTO_INCREMENT PRIMARY KEY,
            role_id INT NOT NULL,
            menu_id INT NOT NULL,
            is_granted BOOLEAN DEFAULT TRUE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
            created_by INT,
            INDEX idx_role_id (role_id),
            INDEX idx_menu_id (menu_id),
            INDEX idx_is_granted (is_granted),
            UNIQUE KEY uk_role_menu (role_id, menu_id),
            FOREIGN KEY (role_id) REFERENCES roles(id) ON DELETE CASCADE,
            FOREIGN KEY (menu_id) REFERENCES menus(id) ON DELETE CASCADE
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
    )";

    std::string createIndexes = R"(
        CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
        CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
        CREATE INDEX IF NOT EXISTS idx_documents_owner ON documents(owner_id);
        CREATE INDEX IF NOT EXISTS idx_documents_title ON documents(title);
        CREATE INDEX IF NOT EXISTS idx_documents_created ON documents(created_at);
        CREATE INDEX IF NOT EXISTS idx_document_shares_shared_to ON document_shares(shared_to_user_id);
        CREATE INDEX IF NOT EXISTS idx_document_shares_shared_by ON document_shares(shared_by_user_id);
        CREATE INDEX IF NOT EXISTS idx_document_shares_document ON document_shares(document_id);
    )";

    // 创建users表
    if (mysql_query(db, createUsersTable.c_str()) != 0) {
        LOG_ERROR("创建users表失败: " + std::string(mysql_error(db)));
        return false;
    }

    // 创建documents表
    if (mysql_query(db, createDocumentsTable.c_str()) != 0) {
        LOG_ERROR("创建documents表失败: " + std::string(mysql_error(db)));
        return false;
    }

    // 创建document_shares表
    if (mysql_query(db, createDocumentSharesTable.c_str()) != 0) {
        LOG_ERROR("创建document_shares表失败: " + std::string(mysql_error(db)));
        return false;
    }

    // 创建权限管理相关表
    if (mysql_query(db, createRolesTable.c_str()) != 0) {
        LOG_ERROR("创建roles表失败: " + std::string(mysql_error(db)));
        return false;
    }

    if (mysql_query(db, createMenusTable.c_str()) != 0) {
        LOG_ERROR("创建menus表失败: " + std::string(mysql_error(db)));
        return false;
    }

    if (mysql_query(db, createUserRolesTable.c_str()) != 0) {
        LOG_ERROR("创建user_roles表失败: " + std::string(mysql_error(db)));
        return false;
    }

    if (mysql_query(db, createRoleMenusTable.c_str()) != 0) {
        LOG_ERROR("创建role_menus表失败: " + std::string(mysql_error(db)));
        return false;
    }

    // 创建用户权限视图
    std::string createUserPermissionsView = R"(
        CREATE OR REPLACE VIEW user_permissions AS
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
    )";

    if (mysql_query(db, createUserPermissionsView.c_str()) != 0) {
        LOG_ERROR("创建user_permissions视图失败: " + std::string(mysql_error(db)));
        return false;
    }

    return true;
}

Result<User> DatabaseManager::createUser(const std::string& username, const std::string& passwordHash, const std::string& email) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<User>::Error("数据库未连接");
    }
    
    // 构建插入SQL语句
    std::string sql = "INSERT INTO users (username, password_hash, email) VALUES ('" + 
                      username + "', '" + passwordHash + "', '" + email + "');";
    
    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<User>::Error("插入用户失败: " + std::string(mysql_error(db)));
    }
    
    // 获取新插入的用户id
    int userId = (int)mysql_insert_id(db);

    Result<User> result= getUserById(userId);  // 调用查询函数获取新用户信息
    // 查询新用户信息
    return result;
}

Result<User> DatabaseManager::getUserByUsername(const std::string& username) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<User>::Error("数据库未连接");
    }
    
    std::string sql = "SELECT id, username, password_hash, email, created_at, last_login, is_active FROM users WHERE username = '" + username + "';";
    
    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<User>::Error("查询用户失败: " + std::string(mysql_error(db)));
    }
    
    MYSQL_RES* result = mysql_store_result(db);
    if (!result) {
        return Result<User>::Error("获取查询结果失败: " + std::string(mysql_error(db)));
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        User user;
        user.id = std::stoi(row[0]);
        user.username = row[1];
        user.password_hash = row[2];
        user.email = row[3];
        // created_at 和 last_login 暂时使用当前时间，后续可以解析时间字符串
        user.created_at = std::chrono::system_clock::now();
        user.last_login = std::chrono::system_clock::now();
        user.is_active = std::stoi(row[6]) != 0;
        
        mysql_free_result(result);
        return Result<User>::Success(user);
    } else {
        mysql_free_result(result);
        return Result<User>::Error("未找到该用户");
    }
}

Result<User> DatabaseManager::getUserById(int userId) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<User>::Error("数据库未连接");
    }
    
    std::string sql = "SELECT id, username, password_hash, email, created_at, last_login, is_active FROM users WHERE id = " + std::to_string(userId) + ";";
    
    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<User>::Error("查询用户失败: " + std::string(mysql_error(db)));
    }
    
    MYSQL_RES* result = mysql_store_result(db);
    if (!result) {
        return Result<User>::Error("获取查询结果失败: " + std::string(mysql_error(db)));
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        User user;
        user.id = std::stoi(row[0]);
        user.username = row[1];
        user.password_hash = row[2];
        user.email = row[3];
        // created_at 和 last_login 暂时使用当前时间，后续可以解析时间字符串
        user.created_at = std::chrono::system_clock::now();
        user.last_login = std::chrono::system_clock::now();
        user.is_active = std::stoi(row[6]) != 0;
        
        mysql_free_result(result);
        return Result<User>::Success(user);
    } else {
        mysql_free_result(result);
        return Result<User>::Error("未找到该用户");
    }
}

Result<std::vector<User>> DatabaseManager::getAllUsers(int limit, int offset) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<std::vector<User>>::Error("数据库未连接");
    }
    
    std::string sql = "SELECT id, username, password_hash, email, created_at, last_login, is_active FROM users LIMIT " + 
                      std::to_string(limit) + " OFFSET " + std::to_string(offset) + ";";
    
    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<std::vector<User>>::Error("查询用户失败: " + std::string(mysql_error(db)));
    }
    
    MYSQL_RES* result = mysql_store_result(db);
    if (!result) {
        return Result<std::vector<User>>::Error("获取查询结果失败: " + std::string(mysql_error(db)));
    }
    
    std::vector<User> users;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        User user;
        user.id = std::stoi(row[0]);
        user.username = row[1];
        user.password_hash = row[2];
        user.email = row[3];
        user.created_at = std::chrono::system_clock::now();
        user.last_login = std::chrono::system_clock::now();
        user.is_active = std::stoi(row[6]) != 0;
        users.push_back(user);
    }
    
    mysql_free_result(result);
    return Result<std::vector<User>>::Success(users);
}

Result<bool> DatabaseManager::updateUser(const User& user) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<bool>::Error("数据库未连接");
    }
    
    std::string sql = "UPDATE users SET username = '" + user.username + 
                      "', password_hash = '" + user.password_hash + 
                      "', email = '" + user.email + 
                      "', is_active = " + (user.is_active ? "1" : "0") + 
                      " WHERE id = " + std::to_string(user.id) + ";";
    
    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<bool>::Error("更新用户失败: " + std::string(mysql_error(db)));
    }
    
    //if (mysql_affected_rows(db) == 0) {
    //    return Result<bool>::Error("用户不存在或未发生更改");
    //}
    
    return Result<bool>::Success(true);
}

Result<bool> DatabaseManager::deleteUser(int userId) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<bool>::Error("数据库未连接");
    }
    
    std::string sql = "DELETE FROM users WHERE id = " + std::to_string(userId) + ";";
    
    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<bool>::Error("删除用户失败: " + std::string(mysql_error(db)));
    }
    
    if (mysql_affected_rows(db) == 0) {
        return Result<bool>::Error("用户不存在");
    }
    
    return Result<bool>::Success(true);
}

Result<bool> DatabaseManager::updateUserLastLogin(int userId) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<bool>::Error("数据库未连接");
    }
    
    std::string sql = "UPDATE users SET last_login = CURRENT_TIMESTAMP WHERE id = " + std::to_string(userId) + ";";
    
    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<bool>::Error("更新最后登录时间失败: " + std::string(mysql_error(db)));
    }
    
    return Result<bool>::Success(true);
}

Result<Document> DatabaseManager::createDocument(const std::string& title, const std::string& description,
                                                const std::string& filePath, const std::string& minioKey,
                                                int ownerId, size_t fileSize, const std::string& contentType) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<Document>::Error("数据库未连接");
    }
    
    std::string sql = "INSERT INTO documents (title, description, file_path, minio_key, owner_id, file_size, content_type) VALUES ('" +
                      title + "', '" + description + "', '" + filePath + "', '" + minioKey + "', " +
                      std::to_string(ownerId) + ", " + std::to_string(fileSize) + ", '" + contentType + "');";
    
    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<Document>::Error("插入文档失败: " + std::string(mysql_error(db)));
    }
    
    int docId = (int)mysql_insert_id(db);
    return getDocumentById(docId);
}

Result<Document> DatabaseManager::getDocumentById(int docId) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<Document>::Error("数据库未连接");
    }
    
    std::string sql = "SELECT id, title, description, file_path, minio_key, owner_id, created_at, updated_at, file_size, content_type FROM documents WHERE id = " + std::to_string(docId) + ";";
    
    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<Document>::Error("查询文档失败: " + std::string(mysql_error(db)));
    }
    
    MYSQL_RES* result = mysql_store_result(db);
    if (!result) {
        return Result<Document>::Error("获取查询结果失败: " + std::string(mysql_error(db)));
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        Document doc;
        doc.id = std::stoi(row[0]);
        doc.title = row[1];
        doc.description = row[2] ? row[2] : "";
        doc.file_path = row[3] ? row[3] : "";
        doc.minio_key = row[4] ? row[4] : "";
        doc.owner_id = std::stoi(row[5]);
        doc.created_at = std::chrono::system_clock::now();
        doc.updated_at = std::chrono::system_clock::now();
        doc.file_size = std::stoull(row[8] ? row[8] : "0");
        doc.content_type = row[9] ? row[9] : "";
        
        mysql_free_result(result);
        return Result<Document>::Success(doc);
    } else {
        mysql_free_result(result);
        return Result<Document>::Error("未找到该文档");
    }
}

Result<std::vector<Document>> DatabaseManager::getDocumentsByOwner(int ownerId, int limit, int offset) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<std::vector<Document>>::Error("数据库未连接");
    }
    
    std::string sql = "SELECT id, title, description, file_path, minio_key, owner_id, created_at, updated_at, file_size, content_type FROM documents WHERE owner_id = " + 
                      std::to_string(ownerId) + " LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset) + ";";
    
    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<std::vector<Document>>::Error("查询文档失败: " + std::string(mysql_error(db)));
    }
    
    MYSQL_RES* result = mysql_store_result(db);
    if (!result) {
        return Result<std::vector<Document>>::Error("获取查询结果失败: " + std::string(mysql_error(db)));
    }
    
    std::vector<Document> documents;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Document doc;
        doc.id = std::stoi(row[0]);
        doc.title = row[1];
        doc.description = row[2] ? row[2] : "";
        doc.file_path = row[3] ? row[3] : "";
        doc.minio_key = row[4] ? row[4] : "";
        doc.owner_id = std::stoi(row[5]);
        doc.created_at = Utils::parseTimestamp(row[6] ? row[6] : "");
        doc.updated_at = Utils::parseTimestamp(row[7] ? row[7] : "");
        doc.file_size = std::stoull(row[8] ? row[8] : "0");
        doc.content_type = row[9] ? row[9] : "";
        documents.push_back(doc);
    }
    
    mysql_free_result(result);
    return Result<std::vector<Document>>::Success(documents);
}

Result<std::vector<Document>> DatabaseManager::getAllDocuments(int limit, int offset) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<std::vector<Document>>::Error("数据库未连接");
    }
    
    std::string sql = "SELECT id, title, description, file_path, minio_key, owner_id, created_at, updated_at, file_size, content_type FROM documents LIMIT " + 
                      std::to_string(limit) + " OFFSET " + std::to_string(offset) + ";";
    
    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<std::vector<Document>>::Error("查询文档失败: " + std::string(mysql_error(db)));
    }
    
    MYSQL_RES* result = mysql_store_result(db);
    if (!result) {
        return Result<std::vector<Document>>::Error("获取查询结果失败: " + std::string(mysql_error(db)));
    }
    
    std::vector<Document> documents;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Document doc;
        doc.id = std::stoi(row[0]);
        doc.title = row[1];
        doc.description = row[2] ? row[2] : "";
        doc.file_path = row[3] ? row[3] : "";
        doc.minio_key = row[4] ? row[4] : "";
        doc.owner_id = std::stoi(row[5]);
        doc.created_at = Utils::parseTimestamp(row[6] ? row[6] : "");
        doc.updated_at = Utils::parseTimestamp(row[7] ? row[7] : "");
        doc.file_size = std::stoull(row[8] ? row[8] : "0");
        doc.content_type = row[9] ? row[9] : "application/octet-stream";
        documents.push_back(doc);
    }
    
    mysql_free_result(result);
    return Result<std::vector<Document>>::Success(documents);
}

Result<bool> DatabaseManager::updateDocument(const Document& doc) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<bool>::Error("数据库未连接");
    }
    
    std::string sql = "UPDATE documents SET title = '" + doc.title +
                      "', description = '" + doc.description +
                      "', file_path = '" + doc.file_path +
                      "', minio_key = '" + doc.minio_key +
                      "', file_size = " + std::to_string(doc.file_size) +
                      ", content_type = '" + doc.content_type +
                      "' WHERE id = " + std::to_string(doc.id) + ";";
    
    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<bool>::Error("更新文档失败: " + std::string(mysql_error(db)));
    }
    
    if (mysql_affected_rows(db) == 0) {
        return Result<bool>::Error("文档不存在或未发生更改");
    }
    
    return Result<bool>::Success(true);
}

Result<bool> DatabaseManager::deleteDocument(int docId) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<bool>::Error("数据库未连接");
    }
    
    std::string sql = "DELETE FROM documents WHERE id = " + std::to_string(docId) + ";";
    
    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<bool>::Error("删除文档失败: " + std::string(mysql_error(db)));
    }
    
    if (mysql_affected_rows(db) == 0) {
        return Result<bool>::Error("文档不存在");
    }
    
    return Result<bool>::Success(true);
}

// Document sharing operations
Result<DocumentShare> DatabaseManager::createDocumentShare(int documentId, int sharedByUserId, int sharedToUserId,
                                                          int sharedDocumentId, const std::string& sharedMinioKey) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<DocumentShare>::Error("数据库未连接");
    }

    std::string sql = "INSERT INTO document_shares (document_id, shared_by_user_id, shared_to_user_id, shared_document_id, shared_minio_key) VALUES (" +
                      std::to_string(documentId) + ", " + std::to_string(sharedByUserId) + ", " +
                      std::to_string(sharedToUserId) + ", " + std::to_string(sharedDocumentId) + ", '" +
                      sharedMinioKey + "');";

    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<DocumentShare>::Error("创建分享记录失败: " + std::string(mysql_error(db)));
    }

    // 获取新插入的分享记录ID
    int shareId = (int)mysql_insert_id(db);

    // 查询新创建的分享记录
    std::string selectSql = "SELECT id, document_id, shared_by_user_id, shared_to_user_id, shared_document_id, shared_minio_key, created_at FROM document_shares WHERE id = " +
                           std::to_string(shareId) + ";";

    if (mysql_query(db, selectSql.c_str()) != 0) {
        return Result<DocumentShare>::Error("查询分享记录失败: " + std::string(mysql_error(db)));
    }

    MYSQL_RES* result = mysql_store_result(db);
    if (!result) {
        return Result<DocumentShare>::Error("获取查询结果失败: " + std::string(mysql_error(db)));
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return Result<DocumentShare>::Error("分享记录不存在");
    }

    DocumentShare share;
    share.id = std::stoi(row[0]);
    share.document_id = std::stoi(row[1]);
    share.shared_by_user_id = std::stoi(row[2]);
    share.shared_to_user_id = std::stoi(row[3]);
    share.shared_document_id = std::stoi(row[4]);
    share.shared_minio_key = row[5] ? row[5] : "";
    share.created_at = Utils::parseTimestamp(row[6] ? row[6] : "");

    mysql_free_result(result);
    return Result<DocumentShare>::Success(share);
}

Result<std::vector<Document>> DatabaseManager::getSharedDocuments(int userId, int limit, int offset) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<std::vector<Document>>::Error("数据库未连接");
    }

    std::string sql = "SELECT d.id, d.title, d.description, d.file_path, ds.shared_minio_key, d.owner_id, d.created_at, d.updated_at, d.file_size, d.content_type "
                      "FROM documents d "
                      "INNER JOIN document_shares ds ON d.id = ds.shared_document_id "
                      "WHERE ds.shared_to_user_id = " + std::to_string(userId) +
                      " LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset) + ";";

    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<std::vector<Document>>::Error("查询分享文档失败: " + std::string(mysql_error(db)));
    }

    MYSQL_RES* result = mysql_store_result(db);
    if (!result) {
        return Result<std::vector<Document>>::Error("获取查询结果失败: " + std::string(mysql_error(db)));
    }

    std::vector<Document> documents;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Document doc;
        doc.id = std::stoi(row[0]);
        doc.title = row[1];
        doc.description = row[2] ? row[2] : "";
        doc.file_path = row[3] ? row[3] : "";
        doc.minio_key = row[4] ? row[4] : "";  // 使用分享的minio_key
        doc.owner_id = std::stoi(row[5]);
        doc.created_at = Utils::parseTimestamp(row[6] ? row[6] : "");
        doc.updated_at = Utils::parseTimestamp(row[7] ? row[7] : "");
        doc.file_size = std::stoull(row[8] ? row[8] : "0");
        doc.content_type = row[9] ? row[9] : "application/octet-stream";
        documents.push_back(doc);
    }

    mysql_free_result(result);
    return Result<std::vector<Document>>::Success(documents);
}

Result<std::vector<DocumentShare>> DatabaseManager::getDocumentShares(int documentId) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<std::vector<DocumentShare>>::Error("数据库未连接");
    }

    std::string sql = "SELECT id, document_id, shared_by_user_id, shared_to_user_id, shared_document_id, shared_minio_key, created_at FROM document_shares WHERE document_id = " +
                      std::to_string(documentId) + ";";

    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<std::vector<DocumentShare>>::Error("查询分享记录失败: " + std::string(mysql_error(db)));
    }

    MYSQL_RES* result = mysql_store_result(db);
    if (!result) {
        return Result<std::vector<DocumentShare>>::Error("获取查询结果失败: " + std::string(mysql_error(db)));
    }

    std::vector<DocumentShare> shares;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        DocumentShare share;
        share.id = std::stoi(row[0]);
        share.document_id = std::stoi(row[1]);
        share.shared_by_user_id = std::stoi(row[2]);
        share.shared_to_user_id = std::stoi(row[3]);
        share.shared_document_id = std::stoi(row[4]);
        share.shared_minio_key = row[5] ? row[5] : "";
        share.created_at = Utils::parseTimestamp(row[6] ? row[6] : "");
        shares.push_back(share);
    }

    mysql_free_result(result);
    return Result<std::vector<DocumentShare>>::Success(shares);
}

Result<bool> DatabaseManager::deleteDocumentShare(int shareId) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<bool>::Error("数据库未连接");
    }

    std::string sql = "DELETE FROM document_shares WHERE id = " + std::to_string(shareId) + ";";

    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<bool>::Error("删除分享记录失败: " + std::string(mysql_error(db)));
    }

    if (mysql_affected_rows(db) == 0) {
        return Result<bool>::Error("分享记录不存在");
    }

    return Result<bool>::Success(true);
}

Result<bool> DatabaseManager::isDocumentShared(int documentId, int sharedByUserId, int sharedToUserId) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<bool>::Error("数据库未连接");
    }

    std::string sql = "SELECT COUNT(*) FROM document_shares WHERE document_id = " + std::to_string(documentId) +
                      " AND shared_by_user_id = " + std::to_string(sharedByUserId) +
                      " AND shared_to_user_id = " + std::to_string(sharedToUserId) + ";";

    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<bool>::Error("查询分享记录失败: " + std::string(mysql_error(db)));
    }

    MYSQL_RES* result = mysql_store_result(db);
    if (!result) {
        return Result<bool>::Error("获取查询结果失败: " + std::string(mysql_error(db)));
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    bool exists = false;
    if (row && row[0]) {
        int count = std::stoi(row[0]);
        exists = (count > 0);
    }

    mysql_free_result(result);
    return Result<bool>::Success(exists);
}

Result<std::vector<User>> DatabaseManager::searchUsers(const std::string& query, int limit) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<std::vector<User>>::Error("数据库未连接");
    }
    
    std::string sql = "SELECT id, username, password_hash, email, created_at, last_login, is_active FROM users WHERE username LIKE '%" + 
                      query + "%' OR email LIKE '%" + query + "%' LIMIT " + std::to_string(limit) + ";";
    
    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<std::vector<User>>::Error("搜索用户失败: " + std::string(mysql_error(db)));
    }
    
    MYSQL_RES* result = mysql_store_result(db);
    if (!result) {
        return Result<std::vector<User>>::Error("获取查询结果失败: " + std::string(mysql_error(db)));
    }
    
    std::vector<User> users;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        User user;
        user.id = std::stoi(row[0]);
        user.username = row[1];
        user.password_hash = row[2];
        user.email = row[3];
        user.created_at = std::chrono::system_clock::now();
        user.last_login = std::chrono::system_clock::now();
        user.is_active = std::stoi(row[6]) != 0;
        users.push_back(user);
    }
    
    mysql_free_result(result);
    return Result<std::vector<User>>::Success(users);
}

Result<std::vector<Document>> DatabaseManager::searchDocuments(const std::string& query, int limit) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<std::vector<Document>>::Error("数据库未连接");
    }
    
    std::string sql = "SELECT id, title, description, file_path, minio_key, owner_id, created_at, updated_at, file_size, content_type FROM documents WHERE title LIKE '%" + 
                      query + "%' OR description LIKE '%" + query + "%' LIMIT " + std::to_string(limit) + ";";
    
    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<std::vector<Document>>::Error("搜索文档失败: " + std::string(mysql_error(db)));
    }
    
    MYSQL_RES* result = mysql_store_result(db);
    if (!result) {
        return Result<std::vector<Document>>::Error("获取查询结果失败: " + std::string(mysql_error(db)));
    }
    
    std::vector<Document> documents;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        Document doc;
        doc.id = std::stoi(row[0]);
        doc.title = row[1];
        doc.description = row[2] ? row[2] : "";
        doc.file_path = row[3] ? row[3] : "";
        doc.minio_key = row[4] ? row[4] : "";
        doc.owner_id = std::stoi(row[5]);
        doc.created_at = Utils::parseTimestamp(row[6] ? row[6] : "");
        doc.updated_at = Utils::parseTimestamp(row[7] ? row[7] : "");
        doc.file_size = std::stoull(row[8] ? row[8] : "0");
        doc.content_type = row[9] ? row[9] : "application/octet-stream";
        documents.push_back(doc);
    }
    
    mysql_free_result(result);
    return Result<std::vector<Document>>::Success(documents);
}

Result<int> DatabaseManager::getUserCount() {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<int>::Error("数据库未连接");
    }
    
    std::string sql = "SELECT COUNT(*) FROM users;";
    
    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<int>::Error("获取用户数量失败: " + std::string(mysql_error(db)));
    }
    
    MYSQL_RES* result = mysql_store_result(db);
    if (!result) {
        return Result<int>::Error("获取查询结果失败: " + std::string(mysql_error(db)));
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    int count = 0;
    if (row) {
        count = std::stoi(row[0]);
    }
    
    mysql_free_result(result);
    return Result<int>::Success(count);
}

Result<int> DatabaseManager::getDocumentCount() {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<int>::Error("数据库未连接");
    }
    
    std::string sql = "SELECT COUNT(*) FROM documents;";
    
    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<int>::Error("获取文档数量失败: " + std::string(mysql_error(db)));
    }
    
    MYSQL_RES* result = mysql_store_result(db);
    if (!result) {
        return Result<int>::Error("获取查询结果失败: " + std::string(mysql_error(db)));
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    int count = 0;
    if (row) {
        count = std::stoi(row[0]);
    }
    
    mysql_free_result(result);
    return Result<int>::Success(count);
}

Result<size_t> DatabaseManager::getTotalFileSize() {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<size_t>::Error("数据库未连接");
    }
    
    std::string sql = "SELECT SUM(file_size) FROM documents;";
    
    if (mysql_query(db, sql.c_str()) != 0) {
        return Result<size_t>::Error("获取总文件大小失败: " + std::string(mysql_error(db)));
    }
    
    MYSQL_RES* result = mysql_store_result(db);
    if (!result) {
        return Result<size_t>::Error("获取查询结果失败: " + std::string(mysql_error(db)));
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    size_t totalSize = 0;
    if (row && row[0]) {
        totalSize = std::stoull(row[0]);
    }
    
    mysql_free_result(result);
    return Result<size_t>::Success(totalSize);
}

bool DatabaseManager::beginTransaction() {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return false;
    }
    
    return mysql_query(db, "START TRANSACTION;") == 0;
}

bool DatabaseManager::commitTransaction() {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return false;
    }
    
    return mysql_query(db, "COMMIT;") == 0;
}

bool DatabaseManager::rollbackTransaction() {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return false;
    }
    
    return mysql_query(db, "ROLLBACK;") == 0;
}

Result<bool> DatabaseManager::vacuum() {
    // MySQL 不支持 VACUUM，这里返回成功
    return Result<bool>::Success(true, "MySQL 不需要 VACUUM 操作");
}

Result<std::vector<std::map<std::string, std::string>>> DatabaseManager::executeQuery(const std::string& query) {
    std::lock_guard<std::recursive_mutex> lock(dbMutex);
    if (!isConnected || db == nullptr) {
        return Result<std::vector<std::map<std::string, std::string>>>::Error("数据库未连接");
    }
    
    if (mysql_query(db, query.c_str()) != 0) {
        return Result<std::vector<std::map<std::string, std::string>>>::Error("执行查询失败: " + std::string(mysql_error(db)));
    }
    
    MYSQL_RES* result = mysql_store_result(db);
    if (!result) {
        // 检查是否有错误
        if (mysql_errno(db) != 0) {
            return Result<std::vector<std::map<std::string, std::string>>>::Error("获取查询结果失败: " + std::string(mysql_error(db)));
        }
        // 如果没有错误，说明这是一个不返回结果集的查询（如INSERT、UPDATE、DELETE）
        // 获取受影响的行数
        my_ulonglong affectedRows = mysql_affected_rows(db);
        return Result<std::vector<std::map<std::string, std::string>>>::Success(std::vector<std::map<std::string, std::string>>());
    }
    
    std::vector<std::map<std::string, std::string>> rows;
    MYSQL_FIELD* fields = mysql_fetch_fields(result);
    int numFields = mysql_num_fields(result);
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        std::map<std::string, std::string> rowData;
        for (int i = 0; i < numFields; i++) {
            std::string fieldName = fields[i].name;
            std::string fieldValue = row[i] ? row[i] : "";
            rowData[fieldName] = fieldValue;
        }
        rows.push_back(rowData);
    }
    
    mysql_free_result(result);
    return Result<std::vector<std::map<std::string, std::string>>>::Success(rows);
}

std::string DatabaseManager::getLastError() const {
    if (db) {
        return std::string(mysql_error(db));
    }
    return "数据库未连接";
}
