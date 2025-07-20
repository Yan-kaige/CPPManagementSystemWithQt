#include "CLIHandler.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <limits>
#include <thread>
#include <atomic>
#include "linenoise.h"
#include "Common.h"
#include <QDebug>
CLIHandler* CLIHandler::instance = nullptr;

CLIHandler::CLIHandler()
        : running(false) {
    instance = this; // 设置静态指针，供补全回调用
    dbManager = std::make_unique<DatabaseManager>();
    redisManager = std::make_unique<RedisManager>();
    authManager = std::make_unique<AuthManager>(dbManager.get(), redisManager.get());  // ✅ 传递RedisManager
    minioClient = std::make_unique<MinioClient>();
    importExportManager = std::make_unique<ImportExportManager>(dbManager.get());  // ✅ 传参

    // 加载linenoise历史
    linenoiseHistoryLoad(".cli_history");


    // 启动定时线程，定期清理过期会话（每60秒）
    cleanupThreadRunning = true;
    cleanupThread = std::thread([this]() {
        while (cleanupThreadRunning) {
            std::this_thread::sleep_for(std::chrono::seconds(60));
            if (!cleanupThreadRunning) break;
            authManager->cleanupExpiredSessions();
        }
    });
}

CLIHandler::~CLIHandler() {
    shutdown();
    // 停止定时线程
    cleanupThreadRunning = false;
    if (cleanupThread.joinable()) {
        cleanupThread.join();
    }
    // 保存linenoise历史
    linenoiseHistorySave(".cli_history");
}

bool CLIHandler::initialize() {
    // 先连接数据库
    ConfigManager* config = ConfigManager::getInstance();
    std::string dbType = config->getDatabaseType();
    bool dbOk = false;
    
    if (dbType == "mysql") {
        dbOk = dbManager->connect(
            config->getMysqlHost(),
            config->getMysqlPort(),
            config->getMysqlUsername(),
            config->getMysqlPassword(),
            config->getMysqlDatabase()
        );
    } else {
        printError("不支持的数据库类型: " + dbType + "，目前只支持 MySQL");
        return false;
    }
    
    if (!dbOk) {
        printError("数据库连接失败！");
        return false;
    }
    
    // 尝试连接Redis（可选）
    bool redisOk = redisManager->connect(
        config->getRedisHost(),
        config->getRedisPort(),
        config->getRedisPassword(),
        config->getRedisDatabase()
    );
    
    if (redisOk) {
        printSuccess("Redis连接成功");
        if (redisManager->ping()) {
            printSuccess("Redis服务正常");
        } else {
            printWarning("Redis连接成功但ping失败");
        }
    } else {
        printWarning("Redis连接失败，将使用本地会话存储");
    }
    
    // 初始化MinioClient
    qDebug() << "正在初始化MinIO客户端...";
    qDebug() << "  Endpoint: " << QString::fromStdString(config->getMinioEndpoint());
    qDebug() << "  Access Key: " << QString::fromStdString(config->getMinioAccessKey());
    qDebug() << "  Bucket: " << QString::fromStdString(config->getMinioBucket());
    qDebug() << "  Secure: " << (config->getMinioSecure() ? "true" : "false");
    
    bool minioOk = minioClient->initialize(
        config->getMinioEndpoint(),
        config->getMinioAccessKey(),
        config->getMinioSecretKey(),
        config->getMinioBucket(),
        config->getMinioSecure()
    );
    
    if (minioOk) {
        printSuccess("Minio连接成功");
        // 检查bucket是否存在，如果不存在则创建
        auto bucketExists = minioClient->bucketExists();
        if (bucketExists.success && !bucketExists.data.value()) {
            printInfo("Bucket不存在，正在创建...");
            auto makeBucket = minioClient->makeBucket();
            if (makeBucket.success) {
                printSuccess("Minio bucket创建成功: " + config->getMinioBucket());
            } else {
                printWarning("Minio bucket创建失败: " + makeBucket.message);
            }
        } else if (bucketExists.success) {
            printSuccess("Minio bucket已存在: " + config->getMinioBucket());
        } else {
            printWarning("Minio bucket检查失败: " + bucketExists.message);
        }
    } else {
        printWarning("Minio连接失败，文件存储功能将不可用");
    }
    
    return true;
}



void CLIHandler::shutdown() {
    LOG_INFO("CLI 正在关闭...");
    // 停止定时线程（防止重复stop）
    cleanupThreadRunning = false;
    if (cleanupThread.joinable()) {
        cleanupThread.join();
    }
}





void CLIHandler::printWarning(const std::string& message) {
#ifdef _WIN32
    qDebug() << QString::fromUtf8("[WARNING] " + message);
#else
    qDebug() << QString::fromUtf8("\033[33m[WARNING]\033[0m " + message);
#endif
}

void CLIHandler::printError(const std::string& message) {
#ifdef _WIN32
    std::cerr << "[ERROR] " << message +"\n";
#else
    std::cerr << "\033[31m[ERROR]\033[0m " << message +"\n";
#endif
}





bool CLIHandler::isRunning() const {
    return running;
}



void CLIHandler::printSuccess(const std::string& message) {
#ifdef _WIN32
    qDebug() << QString::fromUtf8("[SUCCESS] " + message);
#else
    qDebug() << QString::fromUtf8("\033[32m[SUCCESS]\033[0m " + message);
#endif
}

void CLIHandler::printInfo(const std::string& message) {
#ifdef _WIN32
    qDebug() << QString::fromUtf8("[INFO] " + message);
#else
    qDebug() << QString::fromUtf8("\033[36m[INFO]\033[0m " + message);
#endif
}













std::vector<User> CLIHandler::getAllUsersForUI() {
    auto result = authManager->getAllUsers();
    if (result.success) {
        return result.data.value();
    }
    return {};
}

std::vector<User> CLIHandler::getSearchedUsersForUI(const std::string& keyword) {
    auto result = dbManager->searchUsers(keyword, 100);
    if (result.success) {
        return result.data.value();
    }
    return {};
}

std::pair<bool, User> CLIHandler::getCurrentUserForUI() {
    auto result = authManager->getCurrentUser();
    if (result.success) {
        return {true, result.data.value()};
    }
    return {false, User{}};
}

std::vector<Document> CLIHandler::getUserDocsForUI(int userId) {
    auto result = dbManager->getDocumentsByOwner(userId, 100, 0);
    if (result.success) {
        return result.data.value();
    }
    return {};
}

std::vector<Document> CLIHandler::getSearchedDocsForUI(int userId, const std::string& keyword) {
    auto result = dbManager->searchDocuments(keyword, 100);
    if (result.success) {
        std::vector<Document> allDocs = result.data.value();
        std::vector<Document> filtered;
        for (const auto& doc : allDocs) {
            if (doc.owner_id == userId) {
                filtered.push_back(doc);
            }
        }
        return filtered;
    }
    return {};
}

std::vector<Document> CLIHandler::getSharedDocsForUI(int userId) {
    auto result = dbManager->getSharedDocuments(userId, 100, 0);
    if (result.success) {
        return result.data.value();
    }
    return {};
}

bool CLIHandler::handleAddDocument(const std::string& title, const std::string& description, const std::string& filePath) {
    if (!authManager->isCurrentlyLoggedIn()) {
        printError("请先登录");
        return false;
    }

    if (!Utils::fileExists(filePath)) {
        printError("文件不存在: " + filePath);
        return false;
    }

    auto userResult = authManager->getCurrentUser();
    if (!userResult.success) {
        printError("获取用户信息失败: " + userResult.message);
        return false;
    }

    User currentUser = userResult.data.value();

    // 构建 MinIO 对象键
    std::string filename = Utils::getFilename(filePath);
    std::string timestamp = Utils::getCurrentTimestamp();
    std::string minioKey = "documents/" + std::to_string(currentUser.id) + "/" + timestamp + "_" + filename;

    // 上传文件到 MinIO（可选）
    if (minioClient && minioClient->isInitialized()) {
        printInfo("正在上传文件到MinIO...");
        printInfo("  MinIO Key: " + minioKey);
        printInfo("  Local File: " + filePath);

        auto uploadResult = minioClient->putObject(minioKey, filePath);
        if (!uploadResult.success) {
            printError("文件上传失败: " + uploadResult.message);
            return false;
        }

        printSuccess("文件上传成功到MinIO: " + minioKey);
    } else {
        printWarning("MinIO客户端未初始化，跳过文件上传");
    }

    // 获取文件大小（用 std::ifstream）
    std::ifstream fs(filePath, std::ios::binary | std::ios::ate);
    size_t fileSize = fs.is_open() ? static_cast<size_t>(fs.tellg()) : 0;

    // 获取文件名（不包含路径）
    std::string fileName = Utils::getFilename(filePath);

    // 获取文件后缀名（不包含点号）
    std::string fileExtension = Utils::getFileExtension(filePath);
    if (!fileExtension.empty() && fileExtension[0] == '.') {
        fileExtension = fileExtension.substr(1); // 移除点号
    }

    // 保存数据库记录 - file_path存储文件名，content_type存储后缀名
    auto result = dbManager->createDocument(
        title, description, fileName, minioKey, currentUser.id, fileSize, fileExtension
        );

    if (result.success) {
        printSuccess("文档添加成功，ID: " + std::to_string(result.data->id));
        printDocument(result.data.value());
        return true;
    } else {
        printError("文档添加失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleGetDocument(int docId) {
    
    auto result = dbManager->getDocumentById(docId);
    if (result.success) {
        Document doc = result.data.value();
        printDocument(doc);
        return true;
    } else {
        printError("获取文档失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleListDocuments(int limit, int offset) {
    if (!authManager->isCurrentlyLoggedIn()) {
        printError("请先登录");
        return false;
    }
    
    // 获取当前用户
    auto userResult = authManager->getCurrentUser();
    if (!userResult.success) {
        printError("获取用户信息失败: " + userResult.message);
        return false;
    }
    
    User currentUser = userResult.data.value();
    
    auto result = dbManager->getDocumentsByOwner(currentUser.id, limit, offset);
    if (result.success) {
        std::vector<Document> docs = result.data.value();
        qDebug() << QString::fromUtf8("文档列表 (共 " + std::to_string(docs.size()) + " 个文档):");
        qDebug() << "ID\t标题\t\t\t大小\t\t创建时间\n";
        qDebug() << "------------------------------------------------\n";
        
        for (const auto& doc : docs) {
            qDebug() << doc.id << "\t" << doc.title.substr(0, 15) << "\t\t" 
                      << doc.file_size << " bytes\t" 
                      << Utils::formatTimestamp(doc.created_at) +"\n";
        }
        return true;
    } else {
        printError("获取文档列表失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleUpdateDocument(int docId, const std::string& title, const std::string& description, const std::string& newFilePath) {

    // 先获取原文档
    auto getResult = dbManager->getDocumentById(docId);
    if (!getResult.success) {
        printError("文档不存在: " + getResult.message);
        return false;
    }

    Document doc = getResult.data.value();
    doc.title = title;
    doc.description = description;
    doc.updated_at = std::chrono::system_clock::now();

    // 如果提供了新文件路径，更新文件相关信息
    if (!newFilePath.empty()) {
        if (!Utils::fileExists(newFilePath)) {
            printError("文件不存在: " + newFilePath);
            return false;
        }

        // 获取当前用户信息
        auto userResult = authManager->getCurrentUser();
        if (!userResult.success) {
            printError("请先登录");
            return false;
        }
        User currentUser = userResult.data.value();

        // 构建新的 MinIO 对象键
        std::string filename = Utils::getFilename(newFilePath);
        std::string timestamp = Utils::getCurrentTimestamp();
        std::string newMinioKey = "documents/" + std::to_string(currentUser.id) + "/" + timestamp + "_" + filename;

        // 上传新文件到 MinIO
        if (minioClient && minioClient->isInitialized()) {
            printInfo("正在上传新文件到MinIO...");
            printInfo("  MinIO Key: " + newMinioKey);
            printInfo("  Local File: " + newFilePath);

            auto uploadResult = minioClient->putObject(newMinioKey, newFilePath);
            if (!uploadResult.success) {
                printError("新文件上传失败: " + uploadResult.message);
                return false;
            }

            printSuccess("新文件上传成功到MinIO: " + newMinioKey);

            // 删除旧文件（如果存在）
            if (!doc.minio_key.empty()) {
                auto deleteResult = minioClient->removeObject(doc.minio_key);
                if (deleteResult.success) {
                    printInfo("已删除旧文件: " + doc.minio_key);
                } else {
                    printWarning("删除旧文件失败: " + deleteResult.message);
                }
            }
        } else {
            printWarning("MinIO客户端未初始化，跳过文件上传");
        }

        // 获取新文件大小
        std::ifstream fs(newFilePath, std::ios::binary | std::ios::ate);
        size_t newFileSize = fs.is_open() ? static_cast<size_t>(fs.tellg()) : 0;

        // 获取新文件名和后缀
        std::string newFileName = Utils::getFilename(newFilePath);
        std::string newFileExtension = Utils::getFileExtension(newFilePath);
        if (!newFileExtension.empty() && newFileExtension[0] == '.') {
            newFileExtension = newFileExtension.substr(1); // 移除点号
        }

        // 更新文档的文件相关字段
        doc.file_path = newFileName;
        doc.minio_key = newMinioKey;
        doc.file_size = newFileSize;
        doc.content_type = newFileExtension;

        printInfo("文件信息已更新:");
        printInfo("  文件名: " + newFileName);
        printInfo("  文件大小: " + std::to_string(newFileSize) + " bytes");
        printInfo("  文件后缀: " + newFileExtension);
    }

    auto result = dbManager->updateDocument(doc);
    if (result.success) {
        printSuccess("文档更新成功");
        printDocument(doc);
        return true;
    } else {
        printError("文档更新失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleDeleteDocument(int docId) {
    
    // 先获取文档信息
    auto getResult = dbManager->getDocumentById(docId);
    if (!getResult.success) {
        printError("文档不存在: " + getResult.message);
        return false;
    }
    
    Document doc = getResult.data.value();
    
    // 删除MinIO中的文件
    if (minioClient && minioClient->isInitialized() && !doc.minio_key.empty()) {
        auto deleteResult = minioClient->removeObject(doc.minio_key);
        if (!deleteResult.success) {
            printWarning("MinIO文件删除失败: " + deleteResult.message);
        } else {
            printSuccess("MinIO文件删除成功");
        }
    }
    
    // 删除数据库记录
    auto result = dbManager->deleteDocument(docId);
    if (result.success) {
        printSuccess("文档删除成功");
        return true;
    } else {
        printError("文档删除失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleSearchDocuments(const std::string& keyword) {
    auto result = dbManager->searchDocuments(keyword, 20);
    if (result.success) {
        std::vector<Document> docs = result.data.value();
        qDebug() << QString::fromUtf8("搜索结果 (共 " + std::to_string(docs.size()) + " 个文档):");
        qDebug() << "ID\t标题\t\t\t大小\t\t创建时间\n";
        qDebug() << "------------------------------------------------\n";

        for (const auto& doc : docs) {
            qDebug() << doc.id << "\t" << doc.title.substr(0, 15) << "\t\t"
                      << doc.file_size << " bytes\t"
                      << Utils::formatTimestamp(doc.created_at) +"\n";
        }
        return true;
    } else {
        printError("搜索失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleShareDocument(int docId, const std::string& targetUsername) {

    // 获取当前用户
    auto currentUserResult = authManager->getCurrentUser();
    if (!currentUserResult.success) {
        printError("请先登录");
        return false;
    }
    User currentUser = currentUserResult.data.value();

    // 获取目标用户
    auto targetUserResult = dbManager->getUserByUsername(targetUsername);
    if (!targetUserResult.success) {
        printError("目标用户不存在: " + targetUserResult.message);
        return false;
    }
    User targetUser = targetUserResult.data.value();

    // 检查是否分享给自己
    if (currentUser.id == targetUser.id) {
        printError("不能分享给自己");
        return false;
    }

    // 获取文档信息
    auto docResult = dbManager->getDocumentById(docId);
    if (!docResult.success) {
        printError("文档不存在: " + docResult.message);
        return false;
    }
    Document doc = docResult.data.value();

    // 检查是否是文档所有者
    if (doc.owner_id != currentUser.id) {
        printError("您不是此文档的所有者，无法分享");
        return false;
    }

    // 检查是否已经分享给该用户
    auto isSharedResult = dbManager->isDocumentShared(docId, currentUser.id, targetUser.id);
    if (!isSharedResult.success) {
        printError("检查分享状态失败: " + isSharedResult.message);
        return false;
    }

    if (isSharedResult.data.value()) {
        printError("已经分享给该用户");
        return false;
    }

    // 在MinIO中复制文件
    std::string timestamp = Utils::getCurrentTimestamp();
    std::string newMinioKey = "shared/" + std::to_string(targetUser.id) + "/" + timestamp + "_" + Utils::getFilename(doc.file_path);

    if (minioClient && minioClient->isInitialized()) {
        printInfo("正在复制文件到MinIO...");
        printInfo("  源MinIO Key: " + doc.minio_key);
        printInfo("  目标MinIO Key: " + newMinioKey);

        auto copyResult = minioClient->copyObject(doc.minio_key, newMinioKey);
        if (!copyResult.success) {
            printError("文件复制失败: " + copyResult.message);
            return false;
        }

        printSuccess("文件复制成功到MinIO: " + newMinioKey);
    } else {
        printWarning("MinIO客户端未初始化，跳过文件复制");
        return false;
    }

    // 创建分享的文档记录
    auto newDocResult = dbManager->createDocument(
        doc.title + " (分享自 " + currentUser.username + ")",
        doc.description,
        doc.file_path,
        newMinioKey,
        targetUser.id,
        doc.file_size,
        doc.content_type
    );

    if (!newDocResult.success) {
        printError("创建分享文档记录失败: " + newDocResult.message);
        // 删除已复制的MinIO文件
        minioClient->removeObject(newMinioKey);
        return false;
    }

    Document newDoc = newDocResult.data.value();

    // 创建分享记录
    auto shareResult = dbManager->createDocumentShare(
        docId,
        currentUser.id,
        targetUser.id,
        newDoc.id,
        newMinioKey
    );

    if (!shareResult.success) {
        printError("创建分享记录失败: " + shareResult.message);
        // 删除已创建的文档记录和MinIO文件
        dbManager->deleteDocument(newDoc.id);
        minioClient->removeObject(newMinioKey);
        return false;
    }

    printSuccess("文档分享成功！");
    printInfo("  文档ID: " + std::to_string(docId));
    printInfo("  文档标题: " + doc.title);
    printInfo("  分享给: " + targetUser.username);
    return true;
}

bool CLIHandler::handleListSharedDocuments(int limit, int offset) {
    // 获取当前用户
    auto currentUserResult = authManager->getCurrentUser();
    if (!currentUserResult.success) {
        printError("请先登录");
        return false;
    }
    User currentUser = currentUserResult.data.value();

    // 获取分享给当前用户的文档
    auto result = dbManager->getSharedDocuments(currentUser.id, limit, offset);
    if (result.success) {
        std::vector<Document> docs = result.data.value();
        qDebug() << QString::fromUtf8("分享给我的文档 (共 " + std::to_string(docs.size()) + " 个):");
        qDebug() << "ID\t标题\t\t\t大小\t\t分享时间\n";
        qDebug() << "------------------------------------------------\n";

        for (const auto& doc : docs) {
            qDebug() << doc.id << "\t" << doc.title.substr(0, 15) << "\t\t"
                      << doc.file_size << " bytes\t"
                      << Utils::formatTimestamp(doc.created_at) +"\n";
        }
        return true;
    } else {
        printError("获取分享文档失败: " + result.message);
        return false;
    }
}

// 文件管理命令实现
bool CLIHandler::handleUploadFile(const std::string& localPath, const std::string& objectName) {
    if (!minioClient || !minioClient->isInitialized()) {
        printError("MinIO客户端未初始化");
        return false;
    }

    std::string actualObjectName = objectName.empty() ? Utils::getFilename(localPath) : objectName;

    if (!Utils::fileExists(localPath)) {
        printError("文件不存在: " + localPath);
        return false;
    }

    auto result = minioClient->putObject(actualObjectName, localPath);
    if (result.success) {
        printSuccess("文件上传成功: " + actualObjectName);
        return true;
    } else {
        printError("文件上传失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleDownloadFile(const std::string& objectName, const std::string& localPath) {
    if (!minioClient || !minioClient->isInitialized()) {
        printError("MinIO客户端未初始化");
        return false;
    }
    
    auto result = minioClient->getObject(objectName, localPath);
    if (result.success) {
        printSuccess("文件下载成功: " + localPath);
        return true;
    } else {
        printError("文件下载失败: " + result.message);
        return false;
    }
}






void CLIHandler::printDocument(const Document& doc) {
    qDebug() << QString::fromUtf8("\n=== 文档信息 === ID: " + std::to_string(doc.id)
        + " 标题: " + doc.title
        + " 描述: " + doc.description
        + " 文件名: " + doc.file_path
        + " MinIO键: " + doc.minio_key
        + " 所有者ID: " + std::to_string(doc.owner_id)
        + " 文件大小: " + std::to_string(doc.file_size) + " bytes"
        + " 文件后缀: " + doc.content_type
        + " 创建时间: " + Utils::formatTimestamp(doc.created_at)
        + " 更新时间: " + Utils::formatTimestamp(doc.updated_at)
        + "\n==================");
}

bool CLIHandler::handleMinioStatus() {
    qDebug() << "\n=== MinIO 状态检查 ===\n";
    
    if (!minioClient) {
        printError("MinIO客户端未创建");
        return false;
    }
    
    qDebug() << "MinIO客户端初始化状态: " << (minioClient->isInitialized() ? "已初始化" : "未初始化");
    
    if (!minioClient->isInitialized()) {
        printError("MinIO客户端未初始化，无法检查状态");
        return false;
    }
    
    // 检查bucket是否存在
    auto bucketExists = minioClient->bucketExists();
    if (bucketExists.success) {
        printSuccess("Bucket检查成功");
        qDebug() << "当前Bucket存在: " << (bucketExists.data.value() ? "是" : "否");
    } else {
        printError("Bucket检查失败: " + bucketExists.message);
    }
    
    // 列出所有bucket
    auto listBuckets = minioClient->listBuckets();
    if (listBuckets.success) {
        std::vector<std::string> buckets = listBuckets.data.value();
        qDebug() << QString::fromUtf8("所有Bucket列表 (共 " + std::to_string(buckets.size()) + " 个):");
        for (const auto& bucket : buckets) {
            qDebug() << "  - " << bucket +"\n";
        }
    } else {
        printError("获取Bucket列表失败: " + listBuckets.message);
    }
    
    // 列出当前bucket中的对象
    auto listObjects = minioClient->listObjects();
    if (listObjects.success) {
        std::vector<std::string> objects = listObjects.data.value();
        qDebug() << QString::fromUtf8("当前Bucket中的对象 (共 " + std::to_string(objects.size()) + " 个):");
        for (const auto& object : objects) {
            qDebug() << "  - " << object +"\n";
        }
    } else {
        printError("获取对象列表失败: " + listObjects.message);
    }
    
    qDebug() << "=========================\n";
    return true;
}











// GUI友好的方法实现
Result<std::string> CLIHandler::loginUser(const std::string& username, const std::string& password) {
    return authManager->login(username, password);
}

Result<bool> CLIHandler::logoutUser() {
    return authManager->logout();
}

Result<User> CLIHandler::registerUser(const std::string& username, const std::string& password, const std::string& email) {
    return authManager->registerUser(username, password, email);
}

Result<bool> CLIHandler::changeUserPassword(const std::string& oldPassword, const std::string& newPassword) {
    return authManager->changePassword(oldPassword, newPassword);
}

Result<bool> CLIHandler::deleteUser(int userId) {
    return dbManager->deleteUser(userId);
}

Result<bool> CLIHandler::activateUser(int userId) {
    auto userResult = dbManager->getUserById(userId);
    if (!userResult.success) {
        return Result<bool>::Error("用户不存在");
    }

    User user = userResult.data.value();
    user.is_active = true;
    auto updateResult = dbManager->updateUser(user);
    return Result<bool>::Success(updateResult.success);
}

Result<bool> CLIHandler::deactivateUser(int userId) {
    auto userResult = dbManager->getUserById(userId);
    if (!userResult.success) {
        return Result<bool>::Error("用户不存在");
    }

    User user = userResult.data.value();
    user.is_active = false;
    auto updateResult = dbManager->updateUser(user);
    return Result<bool>::Success(updateResult.success);
}


Result<bool> CLIHandler::exportUsersToExcel(const std::string& filePath) {
    auto result = importExportManager->exportUsersToCSV(filePath);
    return Result<bool>::Success(result.success);
}

Result<bool> CLIHandler::importUsersFromExcel(const std::string& filePath) {
    auto result = importExportManager->importDocumentsFromCSV(filePath);
    return Result<bool>::Success(result.success);
}

Result<bool> CLIHandler::exportDocumentsToExcel(const std::string& filePath, bool includeShared) {
    if (!authManager->isCurrentlyLoggedIn()) {
        return Result<bool>::Error("用户未登录");
    }

    auto currentUserResult = authManager->getCurrentUser();
    if (!currentUserResult.success) {
        return Result<bool>::Error("获取当前用户失败");
    }

    User currentUser = currentUserResult.data.value();
    auto result = importExportManager->exportDocumentsToCSV(filePath, currentUser.id);
    return Result<bool>::Success(result.success);
}





void CLIHandler::printUser(const User& user) {
    qDebug() << QString::fromUtf8("==================");
    qDebug() << QString::fromUtf8("用户ID: " + std::to_string(user.id));
    qDebug() << QString::fromUtf8("用户名: " + user.username);
    qDebug() << QString::fromUtf8("邮箱: " + user.email);
    qDebug() << QString::fromUtf8("状态: " + user.is_active );
    qDebug() << QString::fromUtf8("创建时间: " + Utils::formatTimestamp(user.created_at));
    qDebug() << QString::fromUtf8("==================");
}

void CLIHandler::completionCallback(const char* prefix, linenoiseCompletions* lc) {
    // 基本命令补全
    std::vector<std::string> commands = {
        "help", "h", "?",
        "exit", "quit", "q",
        "clear"
    };

    std::string prefixStr(prefix);
    for (const auto& cmd : commands) {
        if (cmd.find(prefixStr) == 0) {  // 如果命令以prefix开头
            linenoiseAddCompletion(lc, cmd.c_str());
        }
    }
}
