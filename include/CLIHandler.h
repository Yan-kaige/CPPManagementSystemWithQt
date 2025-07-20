#pragma once

#include "Common.h"
#include "AuthManager.h"
#include "DatabaseManager.h"
#include "RedisManager.h"
#include "MinioClient.h"
#include "ImportExportManager.h"
#include "ConfigManager.h"
#include "Logger.h"
#include <atomic>
#include <thread>


/**
 * @brief CLI处理器主类
 *
 * 负责处理命令行界面的所有功能，包括：
 * - 用户认证和管理
 * - 文档管理操作
 * - 文件上传下载
 * - Excel导入导出
 * - GUI界面数据提供
 */
class CLIHandler {
private:
    // ==================== 私有成员变量 ====================

    /** @brief 数据库管理器 - 负责所有数据库操作 */
    std::unique_ptr<DatabaseManager> dbManager;

    /** @brief 认证管理器 - 负责用户登录、注册、会话管理 */
    std::unique_ptr<AuthManager> authManager;

    /** @brief Redis管理器 - 负责缓存和会话存储 */
    std::unique_ptr<RedisManager> redisManager;

    /** @brief 导入导出管理器 - 负责Excel/CSV文件处理 */
    std::unique_ptr<ImportExportManager> importExportManager;


    /** @brief CLI运行状态标志 */
    bool running;

    /** @brief 定时清理线程运行标志 */
    std::atomic<bool> cleanupThreadRunning;

    /** @brief 定时清理线程 - 定期清理过期会话 */
    std::thread cleanupThread;

public:
    // ==================== 构造和析构 ====================

    /** @brief 构造函数 - 初始化所有组件和线程 */
    CLIHandler();

    /** @brief 析构函数 - 清理资源和停止线程 */
    ~CLIHandler();

    // ==================== 生命周期管理 ====================

    /** @brief 初始化CLI处理器 - 连接数据库、Redis等 */
    bool initialize();

    /** @brief 运行CLI主循环 - 处理用户输入命令 */
    void run();

    /** @brief 关闭CLI处理器 - 清理资源和停止线程 */
    void shutdown();

    /** @brief MinIO客户端 - 负责文件存储操作 */
    std::unique_ptr<MinioClient> minioClient;


    // ==================== 组件访问器 ====================

    /** @brief 获取MinIO客户端实例 */
    MinioClient* getMinioClient() const { return minioClient.get(); }

    /** @brief 获取数据库管理器实例 */
    DatabaseManager* getDbManager() const { return dbManager.get(); }

    /** @brief 获取认证管理器实例 */
    AuthManager* getAuthManager() const { return authManager.get(); }

    // ==================== 状态查询 ====================

    /** @brief 检查CLI是否正在运行 */
    bool isRunning() const;

    // ==================== 静态成员和回调 ====================

    /** @brief 静态实例指针 - 用于linenoise补全回调访问 */
    static CLIHandler* instance;

    // ==================== GUI数据提供方法 ====================

    /** @brief 获取所有用户列表 - 供GUI界面显示 */
    std::vector<User> getAllUsersForUI();

    /** @brief 根据关键词搜索用户 - 供GUI搜索功能使用 */
    std::vector<User> getSearchedUsersForUI(const std::string& keyword);

    /** @brief 获取当前登录用户信息 - 返回pair<是否成功, 用户对象> */
    std::pair<bool, User> getCurrentUserForUI();

    /** @brief 获取指定用户的文档列表 - 供GUI文档管理使用 */
    std::vector<Document> getUserDocsForUI(int userId);

    /** @brief 搜索指定用户的文档 - 根据关键词过滤用户文档 */
    std::vector<Document> getSearchedDocsForUI(int userId, const std::string& keyword);

    /** @brief 获取分享给指定用户的文档列表 */
    std::vector<Document> getSharedDocsForUI(int userId);

    // ==================== 输出格式化方法 ====================

    /** @brief 打印用户信息 - 格式化输出用户详细信息 */
    void printUser(const User& user);

    /** @brief 打印文档信息 - 格式化输出文档详细信息 */
    void printDocument(const Document& doc);

    /** @brief 打印成功消息 - 绿色输出 */
    void printSuccess(const std::string& message);

    /** @brief 打印错误消息 - 红色输出 */
    void printError(const std::string& message);

    /** @brief 打印警告消息 - 黄色输出 */
    void printWarning(const std::string& message);

    /** @brief 打印信息消息 - 普通输出 */
    void printInfo(const std::string& message);

    // ==================== 用户认证和管理 ====================

    /** @brief 用户登录 - 验证用户名密码并创建会话 */
    Result<std::string> loginUser(const std::string& username, const std::string& password);

    /** @brief 用户登出 - 清除当前会话 */
    Result<bool> logoutUser();

    /** @brief 用户注册 - 创建新用户账户 */
    Result<User> registerUser(const std::string& username, const std::string& password, const std::string& email);

    /** @brief 修改用户密码 - 需要提供旧密码验证 */
    Result<bool> changeUserPassword(const std::string& oldPassword, const std::string& newPassword);

    /** @brief 删除用户 - 管理员功能，删除指定用户 */
    Result<bool> deleteUser(int userId);

    /** @brief 激活用户 - 管理员功能，激活被禁用的用户 */
    Result<bool> activateUser(int userId);

    /** @brief 禁用用户 - 管理员功能，禁用指定用户 */
    Result<bool> deactivateUser(int userId);

    // ==================== 文档管理命令 ====================

    /** @brief 添加文档 - 上传文件并创建文档记录 */
    bool handleAddDocument(const std::string& title, const std::string& description, const std::string& filePath);

    /** @brief 获取文档详情 - 根据文档ID显示详细信息 */
    bool handleGetDocument(int docId);

    /** @brief 列出文档 - 分页显示当前用户的文档列表 */
    bool handleListDocuments(int limit = 50, int offset = 0);

    /** @brief 更新文档 - 修改文档标题、描述或替换文件 */
    bool handleUpdateDocument(int docId, const std::string& title, const std::string& description, const std::string& newFilePath = "");

    /** @brief 删除文档 - 删除文档记录和关联文件 */
    bool handleDeleteDocument(int docId);

    /** @brief 搜索文档 - 根据关键词搜索文档标题和描述 */
    bool handleSearchDocuments(const std::string& keyword);

    /** @brief 分享文档 - 将文档分享给指定用户 */
    bool handleShareDocument(int docId, const std::string& targetUsername);

    /** @brief 列出分享文档 - 显示分享给当前用户的文档 */
    bool handleListSharedDocuments(int limit = 50, int offset = 0);

    // ==================== 文件管理命令 ====================

    /** @brief 上传文件 - 将本地文件上传到MinIO存储 */
    bool handleUploadFile(const std::string& localPath, const std::string& objectName = "");

    /** @brief 下载文件 - 从MinIO存储下载文件到本地 */
    bool handleDownloadFile(const std::string& objectName, const std::string& localPath);

    /** @brief 检查MinIO状态 - 显示MinIO连接和存储状态 */
    bool handleMinioStatus();

    // ==================== Excel导入导出功能 ====================

    /** @brief 导出用户数据到Excel - 将所有用户信息导出为Excel文件 */
    Result<bool> exportUsersToExcel(const std::string& filePath);

    /** @brief 从Excel导入用户数据 - 批量导入用户信息 */
    Result<bool> importUsersFromExcel(const std::string& filePath);

    /** @brief 导出文档数据到Excel - 导出当前用户的文档信息 */
    Result<bool> exportDocumentsToExcel(const std::string& filePath, bool includeShared = false);
};
