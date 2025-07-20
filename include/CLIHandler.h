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
#include "linenoise.h"

// 命令结构体
struct Command {
    std::string name;
    std::string description;
    std::string usage;
    std::vector<std::string> examples;
    std::function<bool(const std::vector<std::string>&)> handler;
    bool requiresAuth;
    std::vector<std::string> aliases;
};

// CLI处理器主类
class CLIHandler {
private:
    // 核心组件
    std::unique_ptr<DatabaseManager> dbManager;
    std::unique_ptr<AuthManager> authManager;
    std::unique_ptr<RedisManager> redisManager;

    std::unique_ptr<ImportExportManager> importExportManager;

    // 命令映射
    std::map<std::string, Command> commands;

    // 运行状态
    bool running;
    std::string prompt;

    // 历史记录
    std::vector<std::string> commandHistory;
    int maxHistorySize;

    // 自动完成
    std::vector<std::string> getCompletions(const std::string& partial);

    // 命令解析
    std::vector<std::string> parseCommand(const std::string& input);
    std::string joinArgs(const std::vector<std::string>& args, size_t start = 0);






    // 命令导出
    bool handleExportCommandsCSV(const std::vector<std::string>& args);

    // 定时清理线程相关成员
    std::atomic<bool> cleanupThreadRunning;
    std::thread cleanupThread;

public:
    CLIHandler();
    ~CLIHandler();

      std::unique_ptr<MinioClient> minioClient;

    // 初始化和运行
    bool initialize();
    void run();
    void shutdown();

    // 获取MinioClient
    MinioClient* getMinioClient() const { return minioClient.get(); }

    // 获取DatabaseManager
    DatabaseManager* getDbManager() const { return dbManager.get(); }

    // 获取AuthManager
    AuthManager* getAuthManager() const { return authManager.get(); }

    // 命令注册
    void registerCommand(const Command& command);
    void registerAllCommands();

    // 交互式功能
    void showWelcome();
    void showPrompt();
    std::string getInput();

    // 批处理模式
    bool executeBatch(const std::string& scriptFile);
    bool executeCommand(const std::string& commandLine);

    // 配置和状态
    void setPrompt(const std::string& newPrompt);
    std::string getPrompt() const;
    bool isRunning() const;

    // 历史记录管理
    void addToHistory(const std::string& command);
    void appendToHistoryFile(const std::string& commandLine); // 新增：写入本地历史文件
    void showHistory() const;
    void clearHistory();
    void saveHistory(const std::string& filename) const;
    bool loadHistory(const std::string& filename);

    // 错误处理
    void handleException(const std::exception& e);
    void handleUnknownCommand(const std::string& command);

    // 工具函数
    static std::string getVersion();
    static void showLicense();
    static void showAbout();

    static void completionCallback(const char* prefix, linenoiseCompletions* lc);
    static CLIHandler* instance; // 用于补全回调访问

    // 公开给UI调用的命令处理
    bool handleLogin(const std::vector<std::string>& args);
    bool handleLogout(const std::vector<std::string>& args);
    bool handleListUsers(const std::vector<std::string>& args);
    std::vector<User> getAllUsersForUI();
    std::vector<User> getSearchedUsersForUI(const std::string& keyword);
    bool handleRegister(const std::vector<std::string>& args);
    // 输出格式化
    void printTable(const std::vector<std::vector<std::string>>& data,
                    const std::vector<std::string>& headers = {});
    void printUser(const User& user);
    void printDocument(const Document& doc);
    void printSuccess(const std::string& message);
    void printError(const std::string& message);
    void printWarning(const std::string& message);
    void printInfo(const std::string& message);

    // 输入验证
    bool validateInput(const std::string& input, const std::string& type);
    std::string getPasswordInput(const std::string& prompt = "密码: ");
    std::string getSecureInput(const std::string& prompt);

    // 命令处理函数
    bool handleHelp(const std::vector<std::string>& args);
    bool handleExit(const std::vector<std::string>& args);
    bool handleClear(const std::vector<std::string>& args);
    bool handleHistory(const std::vector<std::string>& args); // 新增：history命令

    // 用户认证命令
    bool handleWhoami(const std::vector<std::string>& args);
    bool handleChangePassword(const std::vector<std::string>& args);

    // 用户管理命令
    bool handleDeleteUser(const std::vector<std::string>& args);
    bool handleSearchUsers(const std::vector<std::string>& args);
    bool handleDeactivateUser(const std::vector<std::string>& args);
    bool handleActivateUser(const std::vector<std::string>& args);
    bool handleListSessions(const std::vector<std::string>& args);
    bool handleClearSessions(const std::vector<std::string>& args);

    // 文档管理命令
    bool handleAddDocument(const std::vector<std::string>& args);
    bool handleGetDocument(const std::vector<std::string>& args);
    bool handleListDocuments(const std::vector<std::string>& args);
    bool handleUpdateDocument(const std::vector<std::string>& args);
    bool handleDeleteDocument(const std::vector<std::string>& args);
    bool handleSearchDocuments(const std::vector<std::string>& args);
    bool handleShareDocument(const std::vector<std::string>& args);
    bool handleListSharedDocuments(const std::vector<std::string>& args);

    // 文件管理命令
    bool handleUploadFile(const std::vector<std::string>& args);
    bool handleDownloadFile(const std::vector<std::string>& args);
    bool handleListFiles(const std::vector<std::string>& args);
    bool handleDeleteFile(const std::vector<std::string>& args);
    bool handleFileInfo(const std::vector<std::string>& args);
    bool handleMinioStatus(const std::vector<std::string>& args);

    std::pair<bool, User> getCurrentUserForUI();
    std::vector<Document> getUserDocsForUI(int userId);
    std::vector<Document> getSearchedDocsForUI(int userId, const std::string& keyword);
    std::vector<Document> getSharedDocsForUI(int userId);


    // Excel导入导出命令
    bool handleImportUsersExcel(const std::vector<std::string>& args);
    bool handleImportDocumentsExcel(const std::vector<std::string>& args);
    bool handleExportUsersExcel(const std::vector<std::string>& args);
    bool handleExportDocumentsExcel(const std::vector<std::string>& args);
    bool handleGenerateUserTemplate(const std::vector<std::string>& args);
    bool handleGenerateDocumentTemplate(const std::vector<std::string>& args);
    bool handlePreviewExcel(const std::vector<std::string>& args);
    bool handleValidateExcel(const std::vector<std::string>& args);
    bool handleExportDocsExcel(const std::string& username, const std::string& filePath);

};
