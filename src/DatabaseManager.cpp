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
//        if (!createTables()) {
//            LOG_ERROR("创建数据库表失败");
//            disconnect();
//            return false;
//        }

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

    std::string createIndexes = R"(
        CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
        CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
        CREATE INDEX IF NOT EXISTS idx_documents_owner ON documents(owner_id);
        CREATE INDEX IF NOT EXISTS idx_documents_title ON documents(title);
        CREATE INDEX IF NOT EXISTS idx_documents_created ON documents(created_at);
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

    // 创建索引
    if (mysql_query(db, createIndexes.c_str()) != 0) {
        LOG_ERROR("创建索引失败: " + std::string(mysql_error(db)));
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
    
    if (mysql_affected_rows(db) == 0) {
        return Result<bool>::Error("用户不存在或未发生更改");
    }
    
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
        return Result<std::vector<std::map<std::string, std::string>>>::Error("获取查询结果失败: " + std::string(mysql_error(db)));
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
