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
        : running(false), prompt(""), maxHistorySize(100) {
    instance = this; // 设置静态指针，供补全回调用
    dbManager = std::make_unique<DatabaseManager>();
    redisManager = std::make_unique<RedisManager>();
    authManager = std::make_unique<AuthManager>(dbManager.get(), redisManager.get());  // ✅ 传递RedisManager
    minioClient = std::make_unique<MinioClient>();
    importExportManager = std::make_unique<ImportExportManager>(dbManager.get());  // ✅ 传参

    // 加载linenoise历史
    linenoiseHistoryLoad(".cli_history");
    // 注册补全回调
    linenoiseSetCompletionCallback(CLIHandler::completionCallback);

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
//    qDebug() << "正在初始化MinIO客户端...\n";
//    qDebug() << "  Endpoint: " << config->getMinioEndpoint() +"\n";
//    qDebug() << "  Access Key: " << config->getMinioAccessKey() +"\n";
//    qDebug() << "  Bucket: " << config->getMinioBucket() +"\n";
//    qDebug() << "  Secure: " << (config->getMinioSecure() ? "true" : "false") +"\n";
    
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
    
    registerAllCommands();
    return true;
}

void CLIHandler::run() {
    running = true;
    while (running) {
        showPrompt();
        std::string input = getInput();
        if (!executeCommand(input)) {
            printWarning("无效命令，输入 'help' 获取帮助。");
        }
    }
}

void CLIHandler::shutdown() {
    LOG_INFO("CLI 正在关闭...");
    // 停止定时线程（防止重复stop）
    cleanupThreadRunning = false;
    if (cleanupThread.joinable()) {
        cleanupThread.join();
    }
}

void CLIHandler::showPrompt() {
    qDebug() << prompt;
}

std::string CLIHandler::getInput() {
    char* line = linenoise(prompt.c_str());
    std::string input;
    if (line) {
        input = line;
        if (!input.empty()) {
            linenoiseHistoryAdd(line); // 添加到linenoise历史
        }
        free(line);
    }
    return input;
}

bool CLIHandler::executeCommand(const std::string& commandLine) {
    auto args = parseCommand(commandLine);
    if (args.empty()) return false;

    // 不记录 history 命令本身
    if (!(args[0] == "history")) {
        appendToHistoryFile(commandLine);
    }

    auto it = commands.find(args[0]);
    if (it != commands.end()) {
        addToHistory(commandLine);
        return it->second.handler(args);
    } else {
        handleUnknownCommand(args[0]);
        return false;
    }
}

void CLIHandler::registerCommand(const Command& command) {
    commands[command.name] = command;
    for (const auto& alias : command.aliases) {
        commands[alias] = command;
    }
}

void CLIHandler::registerAllCommands() {
    registerCommand({"help", "显示帮助信息", "help [命令名]", {"help", "help login"},
                     [this](const std::vector<std::string>& args) { return handleHelp(args); },
                     false, {"h", "?"}});
    registerCommand({"exit", "退出程序", "exit", {},
                     [this](const std::vector<std::string>&) {
                         running = false;
                         return true;
                     }, false, {"quit", "q"}});
    registerCommand({
        "register",
        "用户注册",
        "register <用户名> <密码> <邮箱>",
        {"register alice 123456 alice@example.com"},
        [this](const std::vector<std::string>& args) { return handleRegister(args); },
        false,
        {}
    });
    registerCommand({
        "login",
        "用户登录",
        "login <用户名> <密码>",
        {"login alice 123456"},
        [this](const std::vector<std::string>& args) { return handleLogin(args); },
        false,
        {}
    });
    registerCommand({
        "logout",
        "用户登出",
        "logout",
        {"logout"},
        [this](const std::vector<std::string>& args) { return handleLogout(args); },
        true,
        {}
    });
    registerCommand({
        "whoami",
        "显示当前用户信息",
        "whoami",
        {"whoami"},
        [this](const std::vector<std::string>& args) { return handleWhoami(args); },
        true,
        {}
    });
    registerCommand({
        "changepass",
        "修改密码",
        "changepass <旧密码> <新密码>",
        {"changepass oldpass newpass"},
        [this](const std::vector<std::string>& args) { return handleChangePassword(args); },
        true,
        {"passwd"}
    });
    registerCommand({
        "users",
        "列出所有用户",
        "users [数量]",
        {"users", "users 10"},
        [this](const std::vector<std::string>& args) { return handleListUsers(args); },
        true,
        {"listusers"}
    });
    registerCommand({
        "search",
        "搜索用户",
        "search <关键词>",
        {"search alice", "search @gmail.com"},
        [this](const std::vector<std::string>& args) { return handleSearchUsers(args); },
        true,
        {"find"}
    });
    registerCommand({
        "deactivate",
        "禁用用户",
        "deactivate <用户ID>",
        {"deactivate 2"},
        [this](const std::vector<std::string>& args) { return handleDeactivateUser(args); },
        true,
        {"disable"}
    });
    registerCommand({
        "activate",
        "激活用户",
        "activate <用户ID>",
        {"activate 2"},
        [this](const std::vector<std::string>& args) { return handleActivateUser(args); },
        true,
        {"enable"}
    });
    registerCommand({
        "deleteuser",
        "删除用户",
        "deleteuser <用户ID>",
        {"deleteuser 2"},
        [this](const std::vector<std::string>& args) { return handleDeleteUser(args); },
        true,
        {"deluser"}
    });
    registerCommand({
        "sessions",
        "显示活跃会话",
        "sessions",
        {"sessions"},
        [this](const std::vector<std::string>& args) { return handleListSessions(args); },
        true,
        {"session"}
    });
    registerCommand({
        "clearsessions",
        "清除所有会话",
        "clearsessions",
        {"clearsessions"},
        [this](const std::vector<std::string>& args) { return handleClearSessions(args); },
        true,
        {"clearsess"}
    });
    
    // 文档管理命令
    registerCommand({
        "adddoc",
        "添加文档",
        "adddoc <标题> <描述> <文件路径>",
        {"adddoc \"我的文档\" \"这是一个测试文档\" /path/to/file.pdf"},
        [this](const std::vector<std::string>& args) { return handleAddDocument(args); },
        true,
        {"adddocument", "upload"}
    });
    registerCommand({
        "getdoc",
        "获取文档信息",
        "getdoc <文档ID>",
        {"getdoc 1"},
        [this](const std::vector<std::string>& args) { return handleGetDocument(args); },
        true,
        {"getdocument", "doc"}
    });
    registerCommand({
        "listdocs",
        "列出文档",
        "listdocs [数量] [偏移量]",
        {"listdocs", "listdocs 10", "listdocs 10 20"},
        [this](const std::vector<std::string>& args) { return handleListDocuments(args); },
        true,
        {"listdocuments", "docs"}
    });
    registerCommand({
        "updatedoc",
        "更新文档信息",
        "updatedoc <文档ID> <标题> <描述>",
        {"updatedoc 1 \"新标题\" \"新描述\""},
        [this](const std::vector<std::string>& args) { return handleUpdateDocument(args); },
        true,
        {"updatedocument", "editdoc"}
    });
    registerCommand({
        "deletedoc",
        "删除文档",
        "deletedoc <文档ID>",
        {"deletedoc 1"},
        [this](const std::vector<std::string>& args) { return handleDeleteDocument(args); },
        true,
        {"deletedocument", "deldoc"}
    });
    registerCommand({
        "searchdocs",
        "搜索文档",
        "searchdocs <关键词>",
        {"searchdocs \"报告\"", "searchdocs \"2024\""},
        [this](const std::vector<std::string>& args) { return handleSearchDocuments(args); },
        true,
        {"searchdocuments", "finddocs"}
    });
    
    // 文件管理命令
    registerCommand({
        "upload",
        "上传文件到MinIO",
        "upload <本地文件路径> [对象名称]",
        {"upload /path/to/file.pdf", "upload /path/to/file.pdf myfile.pdf"},
        [this](const std::vector<std::string>& args) { return handleUploadFile(args); },
        true,
        {"uploadfile"}
    });
    registerCommand({
        "download",
        "从MinIO下载文件",
        "download <对象名称> <本地文件路径>",
        {"download myfile.pdf /path/to/downloaded.pdf"},
        [this](const std::vector<std::string>& args) { return handleDownloadFile(args); },
        true,
        {"downloadfile"}
    });
    registerCommand({
        "listfiles",
        "列出MinIO中的文件",
        "listfiles [前缀]",
        {"listfiles", "listfiles documents/"},
        [this](const std::vector<std::string>& args) { return handleListFiles(args); },
        true,
        {"lsfiles"}
    });
    registerCommand({
        "deletefile",
        "删除MinIO中的文件",
        "deletefile <对象名称>",
        {"deletefile myfile.pdf"},
        [this](const std::vector<std::string>& args) { return handleDeleteFile(args); },
        true,
        {"delfile"}
    });
    registerCommand({
        "fileinfo",
        "获取文件信息",
        "fileinfo <对象名称>",
        {"fileinfo myfile.pdf"},
        [this](const std::vector<std::string>& args) { return handleFileInfo(args); },
        true,
        {"info"}
    });
    
    // MinIO状态检查命令
    registerCommand({
        "minio-status",
        "检查MinIO连接状态",
        "minio-status",
        {"minio-status"},
        [this](const std::vector<std::string>& args) { return handleMinioStatus(args); },
        false,
        {"minio"}
    });
    
    // Excel导入导出命令
    registerCommand({
        "import-users-excel",
        "从Excel文件导入用户数据",
        "import-users-excel <文件路径> [跳过标题行]",
        {"import-users-excel users.xlsx", "import-users-excel users.xlsx true"},
        [this](const std::vector<std::string>& args) { return handleImportUsersExcel(args); },
        true,
        {"importusers", "import-users"}
    });
    registerCommand({
        "export-users-excel",
        "导出用户数据到Excel文件",
        "export-users-excel <文件路径> [包含标题行]",
        {"export-users-excel users_export.xlsx", "export-users-excel users_export.xlsx true"},
        [this](const std::vector<std::string>& args) { return handleExportUsersExcel(args); },
        true,
        {"exportusers", "export-users"}
    });
    registerCommand({
        "import-docs-excel",
        "从Excel文件导入文档数据",
        "import-docs-excel <文件路径> [跳过标题行]",
        {"import-docs-excel documents.xlsx", "import-docs-excel documents.xlsx true"},
        [this](const std::vector<std::string>& args) { return handleImportDocumentsExcel(args); },
        true,
        {"importdocs", "import-documents"}
    });
    registerCommand({
        "export-docs-excel",
        "导出文档数据到Excel文件",
        "export-docs-excel <文件路径> [包含标题行]",
        {"export-docs-excel documents_export.xlsx", "export-docs-excel documents_export.xlsx true"},
        [this](const std::vector<std::string>& args) { return handleExportDocumentsExcel(args); },
        true,
        {"exportdocs", "export-documents"}
    });
    registerCommand({
        "gen-user-template",
        "生成用户导入模板",
        "gen-user-template <文件路径>",
        {"gen-user-template user_template.xlsx"},
        [this](const std::vector<std::string>& args) { return handleGenerateUserTemplate(args); },
        true,
        {"gentemplate", "template-user"}
    });
    registerCommand({
        "gen-doc-template",
        "生成文档导入模板",
        "gen-doc-template <文件路径>",
        {"gen-doc-template document_template.xlsx"},
        [this](const std::vector<std::string>& args) { return handleGenerateDocumentTemplate(args); },
        true,
        {"gentemplate", "template-doc"}
    });
    registerCommand({
        "preview-excel",
        "预览Excel文件内容",
        "preview-excel <文件路径> [最大预览行数]",
        {"preview-excel data.xlsx", "preview-excel data.xlsx 20"},
        [this](const std::vector<std::string>& args) { return handlePreviewExcel(args); },
        true,
        {"preview", "excel-preview"}
    });
    registerCommand({
        "validate-excel",
        "验证Excel文件格式",
        "validate-excel <文件路径> <数据类型>",
        {"validate-excel users.xlsx users", "validate-excel documents.xlsx documents"},
        [this](const std::vector<std::string>& args) { return handleValidateExcel(args); },
        true,
        {"validate", "excel-validate"}
    });
    registerCommand({
        "export-commands-csv",
        "导出所有命令为CSV文件",
        "export-commands-csv <输出文件路径>",
        {"export-commands-csv D://test//commands_export.csv"},
        [this](const std::vector<std::string>& args) { return handleExportCommandsCSV(args); },
        false,
        {}
    });
    registerCommand({
        "history",
        "显示所有历史操作过的命令",
        "history",
        {"history"},
        [this](const std::vector<std::string>& args) { return handleHistory(args); },
        false,
        {}
    });

}

std::vector<std::string> CLIHandler::parseCommand(const std::string& input) {
    std::istringstream iss(input);
    std::vector<std::string> args;
    std::string token;
    while (iss >> token) {
        args.push_back(token);
    }
    return args;
}

void CLIHandler::addToHistory(const std::string& command) {
    commandHistory.push_back(command);
    if (commandHistory.size() > maxHistorySize) {
        commandHistory.erase(commandHistory.begin());
    }
}

void CLIHandler::appendToHistoryFile(const std::string& commandLine) {
    std::ofstream file("command_history.log", std::ios::app);
    if (file.is_open()) {
        file << commandLine +"\n";
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

void CLIHandler::handleUnknownCommand(const std::string& command) {
    printWarning("未知命令: " + command);
}

std::string CLIHandler::getVersion() {
    return "1.0.0";
}

void CLIHandler::showWelcome() {
    qDebug() << QString::fromUtf8("欢迎使用 CLI 管理系统 v" + getVersion());
}

std::string CLIHandler::getPrompt() const {
    return prompt;
}

void CLIHandler::setPrompt(const std::string& newPrompt) {
    prompt = newPrompt;
}

bool CLIHandler::isRunning() const {
    return running;
}

bool CLIHandler::handleRegister(const std::vector<std::string>& args) {
    if (args.size() < 4) {
        printError("用法: register <用户名> <密码> <邮箱>");
        return false;
    }
    std::string username = args[1];
    std::string password = args[2];
    std::string email = args[3];
    auto result = authManager->registerUser(username, password, email);
    if (result.success) {
        printSuccess("注册成功，用户名: " + username);
        return true;
    } else {
        printError("注册失败: " + result.message);
        return false;
    }
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

bool CLIHandler::handleLogin(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        printError("用法: login <用户名> <密码>");
        return false;
    }
    
    std::string username = args[1];
    std::string password = args[2];
    
    auto result = authManager->login(username, password);
    if (result.success) {
        printSuccess("登录成功！会话令牌: " + result.data.value().substr(0, 8) + "...");
        return true;
    } else {
        printError("登录失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleLogout(const std::vector<std::string>& args) {
    if (!authManager->isCurrentlyLoggedIn()) {
        printError("当前未登录");
        return false;
    }
    
    auto result = authManager->logout();
    if (result.success) {
        printSuccess("登出成功");
        return true;
    } else {
        printError("登出失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleWhoami(const std::vector<std::string>& args) {
    if (!authManager->isCurrentlyLoggedIn()) {
        printError("当前未登录");
        return false;
    }
    auto result = authManager->getCurrentUser();
    if (result.success) {
        User user = result.data.value();
        qDebug() << QString::fromUtf8("当前用户信息: ID: " + std::to_string(user.id)
            + " 用户名: " + user.username
            + " 邮箱: " + user.email
            + " 状态: " + (user.is_active ? "激活" : "禁用"));
        return true;
    } else {
        printError("获取用户信息失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleHelp(const std::vector<std::string>& args) {
    if (args.size() > 1) {
        // 显示特定命令的帮助
        std::string commandName = args[1];
        auto it = commands.find(commandName);
        if (it != commands.end()) {
            const Command& cmd = it->second;
            qDebug() << "\n=== " << cmd.name << " 命令帮助 ===\n";
            qDebug() << "命令名  : " << cmd.name +"\n";
            qDebug() << "描述    : " << cmd.description +"\n";
            qDebug() << "用法    : " << cmd.usage +"\n";
            if (!cmd.examples.empty()) {
                qDebug() << "示例    :\n";
                for (const auto& example : cmd.examples) {
                    qDebug() << "  " << example +"\n";
                }
            }
            if (!cmd.aliases.empty()) {
                qDebug() << "别名    : ";
                for (size_t i = 0; i < cmd.aliases.size(); ++i) {
                    if (i > 0) qDebug() << ", ";
                    qDebug() << cmd.aliases[i];
                }
                qDebug() <<"\n";
            }
            qDebug() << "需要登录: " << (cmd.requiresAuth ? "是" : "否");
            qDebug() << "================================\n";
            return true;
        } else {
            printError("未知命令: " + commandName + "，请输入 help 查看所有命令");
            return false;
        }
    } else {
        // 显示所有命令的帮助
        std::string helpText;
        helpText += "\n=== C++ 管理系统命令帮助 ===\n";
        helpText += "版本: " + getVersion() + "\n";
        helpText += "输入 'help <命令名>' 查看特定命令的详细帮助\n\n";
        helpText += "【基础命令】\n";
        helpText += "  help     - 显示帮助信息\n";
        helpText += "  exit     - 退出程序\n";
        helpText += "  whoami   - 显示当前用户信息\n";
        helpText += "\n【用户认证】\n";
        helpText += "  register - 用户注册\n";
        helpText += "  login    - 用户登录\n";
        helpText += "  logout   - 用户登出\n";
        helpText += "  changepass - 修改密码\n";
        helpText += "\n【用户管理】\n";
        helpText += "  users    - 列出所有用户\n";
        helpText += "  search   - 搜索用户\n";
        helpText += "  activate - 激活用户\n";
        helpText += "  deactivate - 禁用用户\n";
        helpText += "  deleteuser - 删除用户\n";
        helpText += "  sessions - 显示活跃会话\n";
        helpText += "  clearsessions - 清除所有会话\n";
        helpText += "\n【文档管理】\n";
        helpText += "  adddoc   - 添加文档\n";
        helpText += "  getdoc   - 获取文档信息\n";
        helpText += "  listdocs - 列出文档\n";
        helpText += "  updatedoc - 更新文档信息\n";
        helpText += "  deletedoc - 删除文档\n";
        helpText += "  searchdocs - 搜索文档\n";
        helpText += "\n【文件管理】\n";
        helpText += "  upload   - 上传文件到MinIO\n";
        helpText += "  download - 从MinIO下载文件\n";
        helpText += "  listfiles - 列出MinIO中的文件\n";
        helpText += "  deletefile - 删除MinIO中的文件\n";
        helpText += "  fileinfo - 获取文件信息\n";
        helpText += "  minio-status - 检查MinIO连接状态\n";
        helpText += "\n【Excel导入导出】\n";
        helpText += "  import-users-excel - 从Excel导入用户数据\n";
        helpText += "  export-users-excel - 导出用户数据到Excel\n";
        helpText += "  import-docs-excel - 从Excel导入文档数据\n";
        helpText += "  export-docs-excel - 导出文档数据到Excel\n";
        helpText += "  gen-user-template - 生成用户导入模板\n";
        helpText += "  gen-doc-template - 生成文档导入模板\n";
        helpText += "  preview-excel - 预览Excel文件内容\n";
        helpText += "  validate-excel - 验证Excel文件格式\n";
        helpText += "\n【快捷别名】\n";
        helpText += "  h, ?     - help 的别名\n";
        helpText += "  q, quit  - exit 的别名\n";
        helpText += "\n=== 使用示例 ===\n";
        helpText += "注册新用户: register alice 123456 alice@example.com\n";
        helpText += "登录系统:   login alice 123456\n";
        helpText += "查看帮助:   help login\n";
        helpText += "退出程序:   exit\n";
        helpText += "生成模板:   gen-user-template users.xlsx\n";
        helpText += "导入用户:   import-users-excel users.xlsx\n";
        helpText += "导出用户:   export-users-excel users_export.xlsx\n";
        helpText += "预览文件:   preview-excel data.xlsx 10\n";
        helpText += "================================\n";
        qDebug() << QString::fromUtf8(helpText);
        return true;
    }
}

bool CLIHandler::handleChangePassword(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        printError("用法: changepass <旧密码> <新密码>");
        return false;
    }
    
    std::string oldPassword = args[1];
    std::string newPassword = args[2];
    
    auto result = authManager->changePassword(oldPassword, newPassword);
    if (result.success) {
        printSuccess("密码修改成功，请重新登录");
        return true;
    } else {
        printError("密码修改失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleListUsers(const std::vector<std::string>& args) {
    int limit = 50; // 默认显示50个用户
    if (args.size() > 1) {
        try {
            limit = std::stoi(args[1]);
        } catch (const std::exception& e) {
            printError("无效的数量参数");
            return false;
        }
    }
    
    auto result = authManager->getAllUsers();
    if (result.success) {
        std::vector<User> users = result.data.value();
        qDebug() << QString::fromUtf8("用户列表 (共 " + std::to_string(users.size()) + " 个用户):");
        qDebug() << "ID\t用户名\t\t邮箱\t\t\t状态 \n";
        qDebug() << "----------------------------------------\n";
        
        int count = 0;
        for (const auto& user : users) {
            if (count >= limit) break;
            qDebug() << user.id << "\t" << user.username << "\t\t" 
                      << user.email << "\t\t" << (user.is_active ? "激活" : "禁用");
            count++;
        }
        return true;
    } else {
        printError("获取用户列表失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleSearchUsers(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printError("用法: search <关键词>");
        return false;
    }
    
    std::string query = args[1];
    auto result = dbManager->searchUsers(query, 20);
    if (result.success) {
        std::vector<User> users = result.data.value();
        qDebug() << QString::fromUtf8("搜索结果 (共 " + std::to_string(users.size()) + " 个用户):");
        qDebug() << "ID\t用户名\t\t邮箱\t\t\t状态\n";
        qDebug() << "----------------------------------------\n";
        
        for (const auto& user : users) {
            qDebug() << user.id << "\t" << user.username << "\t\t" 
                      << user.email << "\t\t" << (user.is_active ? "激活" : "禁用") ;
        }
        return true;
    } else {
        printError("搜索失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleDeactivateUser(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printError("用法: deactivate <用户ID>");
        return false;
    }
    
    int userId;
    try {
        userId = std::stoi(args[1]);
    } catch (const std::exception& e) {
        printError("无效的用户ID");
        return false;
    }
    
    auto result = authManager->deactivateUser(userId);
    if (result.success) {
        printSuccess("用户已禁用");
        return true;
    } else {
        printError("禁用用户失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleActivateUser(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printError("用法: activate <用户ID>");
        return false;
    }
    
    int userId;
    try {
        userId = std::stoi(args[1]);
    } catch (const std::exception& e) {
        printError("无效的用户ID");
        return false;
    }
    
    auto result = authManager->activateUser(userId);
    if (result.success) {
        printSuccess("用户已激活");
        return true;
    } else {
        printError("激活用户失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleDeleteUser(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printError("用法: deleteuser <用户ID>");
        return false;
    }
    
    int userId;
    try {
        userId = std::stoi(args[1]);
    } catch (const std::exception& e) {
        printError("无效的用户ID");
        return false;
    }
    
    auto result = authManager->deleteUser(userId);
    if (result.success) {
        printSuccess("用户已删除");
        return true;
    } else {
        printError("删除用户失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleListSessions(const std::vector<std::string>& args) {
    authManager->listActiveSessions();
    return true;
}

bool CLIHandler::handleClearSessions(const std::vector<std::string>& args) {
    auto result = authManager->invalidateAllSessions();
    if (result.success) {
        printSuccess("所有会话已清除");
        return true;
    } else {
        printError("清除会话失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleAddDocument(const std::vector<std::string>& args) {
    if (args.size() < 4) {
        printError("用法: adddoc <标题> <描述> <文件路径>");
        return false;
    }

    if (!authManager->isCurrentlyLoggedIn()) {
        printError("请先登录");
        return false;
    }

    std::string title = args[1];
    std::string description = args[2];
    std::string filePath = args[3];

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

bool CLIHandler::handleGetDocument(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printError("用法: getdoc <文档ID>");
        return false;
    }
    
    int docId;
    try {
        docId = std::stoi(args[1]);
    } catch (const std::exception& e) {
        printError("无效的文档ID");
        return false;
    }
    
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

bool CLIHandler::handleListDocuments(const std::vector<std::string>& args) {
    if (!authManager->isCurrentlyLoggedIn()) {
        printError("请先登录");
        return false;
    }
    
    int limit = 20;
    int offset = 0;
    
    if (args.size() > 1) {
        try {
            limit = std::stoi(args[1]);
        } catch (const std::exception& e) {
            printError("无效的数量参数");
            return false;
        }
    }
    
    if (args.size() > 2) {
        try {
            offset = std::stoi(args[2]);
        } catch (const std::exception& e) {
            printError("无效的偏移量参数");
            return false;
        }
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

bool CLIHandler::handleUpdateDocument(const std::vector<std::string>& args) {
    if (args.size() < 4) {
        printError("用法: updatedoc <文档ID> <标题> <描述>");
        return false;
    }
    
    int docId;
    try {
        docId = std::stoi(args[1]);
    } catch (const std::exception& e) {
        printError("无效的文档ID");
        return false;
    }
    
    std::string title = args[2];
    std::string description = args[3];
    
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

bool CLIHandler::handleDeleteDocument(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printError("用法: deletedoc <文档ID>");
        return false;
    }
    
    int docId;
    try {
        docId = std::stoi(args[1]);
    } catch (const std::exception& e) {
        printError("无效的文档ID");
        return false;
    }
    
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

bool CLIHandler::handleSearchDocuments(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printError("用法: searchdocs <关键词>");
        return false;
    }
    
    std::string query = args[1];
    auto result = dbManager->searchDocuments(query, 20);
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

// 文件管理命令实现
bool CLIHandler::handleUploadFile(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printError("用法: upload <本地文件路径> [对象名称]");
        return false;
    }
    
    if (!minioClient || !minioClient->isInitialized()) {
        printError("MinIO客户端未初始化");
        return false;
    }
    
    std::string filePath = args[1];
    std::string objectName;
    
    if (args.size() > 2) {
        objectName = args[2];
    } else {
        objectName = Utils::getFilename(filePath);
    }

    if (!Utils::fileExists(filePath)) {
        printError("文件不存在: " + filePath);
        return false;
    }
    
    auto result = minioClient->putObject(objectName, filePath);
    if (result.success) {
        printSuccess("文件上传成功: " + objectName);
        return true;
    } else {
        printError("文件上传失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleDownloadFile(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        printError("用法: download <对象名称> <本地文件路径>");
        return false;
    }
    
    if (!minioClient || !minioClient->isInitialized()) {
        printError("MinIO客户端未初始化");
        return false;
    }
    
    std::string objectName = args[1];
    std::string filePath = args[2];
    
    auto result = minioClient->getObject(objectName, filePath);
    if (result.success) {
        printSuccess("文件下载成功: " + filePath);
        return true;
    } else {
        printError("文件下载失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleListFiles(const std::vector<std::string>& args) {
    if (!minioClient || !minioClient->isInitialized()) {
        printError("MinIO客户端未初始化");
        return false;
    }
    
    std::string prefix = "";
    if (args.size() > 1) {
        prefix = args[1];
    }
    
    auto result = minioClient->listObjects(prefix);
    if (result.success) {
        std::vector<std::string> objects = result.data.value();
        qDebug() << QString::fromUtf8("文件列表 (共 " + std::to_string(objects.size()) + " 个文件):");
        qDebug() << "文件名\n";
        qDebug() << "----------------\n";
        
        for (const auto& object : objects) {
            qDebug() << object +"\n";
        }
        return true;
    } else {
        printError("获取文件列表失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleDeleteFile(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printError("用法: deletefile <对象名称>");
        return false;
    }
    
    if (!minioClient || !minioClient->isInitialized()) {
        printError("MinIO客户端未初始化");
        return false;
    }
    
    std::string objectName = args[1];
    
    auto result = minioClient->removeObject(objectName);
    if (result.success) {
        printSuccess("文件删除成功: " + objectName);
        return true;
    } else {
        printError("文件删除失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleFileInfo(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printError("用法: fileinfo <对象名称>");
        return false;
    }
    
    if (!minioClient || !minioClient->isInitialized()) {
        printError("MinIO客户端未初始化");
        return false;
    }
    
    std::string objectName = args[1];
    
    auto result = minioClient->objectExists(objectName);
    if (result.success && result.data.value()) {
        printSuccess("文件存在: " + objectName);
        // 这里可以添加更多文件信息显示
        return true;
    } else {
        printError("文件不存在: " + objectName);
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

bool CLIHandler::handleMinioStatus(const std::vector<std::string>& args) {
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

// ==================== Excel导入导出命令实现 ====================

bool CLIHandler::handleImportUsersExcel(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printError("用法: import-users-excel <Excel文件路径> [跳过标题行]");
        printInfo("示例: import-users-excel users.xlsx true");
        return false;
    }
    
    std::string filePath = args[1];
    bool skipHeader = true;
    
    if (args.size() > 2) {
        skipHeader = (args[2] == "true" || args[2] == "1");
    }
    
    if (!Utils::fileExists(filePath)) {
        printError("文件不存在: " + filePath);
        return false;
    }
    
    printInfo("开始导入用户数据...");
    printInfo("文件路径: " + filePath);
    printInfo("跳过标题行: " + std::string(skipHeader ? "是" : "否"));
    
    // 设置进度回调
    importExportManager->setProgressCallback([](int current, int total, const std::string& status) {
        if (total > 0) {
            int percentage = (current * 100) / total;
            qDebug() << "\r进度: " << percentage << "% (" << current << "/" << total << ") " << status;
            if (current == total) {
                qDebug() <<"\n";
            }
        }
    });
    
    auto result = importExportManager->importUsersFromCSV(filePath, skipHeader);
    
    if (result.success) {
        ImportResult importResult = result.data.value();
        printSuccess("用户数据导入完成！");
        qDebug() << QString::fromUtf8("总记录数: " + std::to_string(importResult.totalRecords));
        qDebug() << QString::fromUtf8("成功导入: " + std::to_string(importResult.successfulImports));
        qDebug() << QString::fromUtf8("导入失败: " + std::to_string(importResult.failedImports));
        qDebug() << QString::fromUtf8("耗时: " + std::to_string(importResult.duration.count()) + "ms");
        // 新增：无论成功与否都打印错误
        if (!importResult.errors.empty()) {
            printWarning("导入过程中的错误:");
            for (const auto& error : importResult.errors) {
                qDebug() << QString::fromUtf8("  - " + error) +"\n";
            }
        }
        return true;
    } else {
        printError("用户数据导入失败: " + result.message);
        // 新增：失败时也打印所有错误
        if (result.data && !result.data->errors.empty()) {
            printWarning("导入过程中的错误:");
            for (const auto& error : result.data->errors) {
                qDebug() << QString::fromUtf8("  - " + error) +"\n";
            }
        }
        return false;
    }
}

bool CLIHandler::handleExportUsersExcel(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printError("用法: export-users-excel <输出文件路径> [包含标题行]");
        printInfo("示例: export-users-excel users_export.xlsx true");
        return false;
    }
    
    std::string filePath = args[1];
    bool includeHeader = true;
    
    if (args.size() > 2) {
        includeHeader = (args[2] == "true" || args[2] == "1");
    }
    
    printInfo("开始导出用户数据...");
    printInfo("输出文件: " + filePath);
    printInfo("包含标题行: " + std::string(includeHeader ? "是" : "否"));
    
    auto result = importExportManager->exportUsersToCSV(filePath, includeHeader);
    
    if (result.success) {
        ExportResult exportResult = result.data.value();
        printSuccess("用户数据导出完成！");
        qDebug() << "导出记录数: " << exportResult.totalRecords +"\n";
        qDebug() << "文件大小: " << exportResult.fileSize << " bytes\n";
        qDebug() << "文件路径: " << exportResult.filePath +"\n";
        qDebug() << "耗时: " << exportResult.duration.count() << "ms\n";
        return true;
    } else {
        printError("用户数据导出失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleImportDocumentsExcel(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printError("用法: import-docs-excel <Excel文件路径> [跳过标题行]");
        printInfo("示例: import-docs-excel documents.xlsx true");
        return false;
    }
    
    std::string filePath = args[1];
    bool skipHeader = true;
    
    if (args.size() > 2) {
        skipHeader = (args[2] == "true" || args[2] == "1");
    }
    
    if (!Utils::fileExists(filePath)) {
        printError("文件不存在: " + filePath);
        return false;
    }
    
    printInfo("开始导入文档数据...");
    printInfo("文件路径: " + filePath);
    printInfo("跳过标题行: " + std::string(skipHeader ? "是" : "否"));
    
    // 设置进度回调
    importExportManager->setProgressCallback([](int current, int total, const std::string& status) {
        if (total > 0) {
            int percentage = (current * 100) / total;
            qDebug() << "\r进度: " << percentage << "% (" << current << "/" << total << ") " << status;
            if (current == total) {
                qDebug() <<"\n";
            }
        }
    });
    
    auto result = importExportManager->importDocumentsFromCSV(filePath, skipHeader);
    
    if (result.success) {
        ImportResult importResult = result.data.value();
        printSuccess("文档数据导入完成！");
        qDebug() << QString::fromUtf8("总记录数: " + std::to_string(importResult.totalRecords));
        qDebug() << QString::fromUtf8("成功导入: " + std::to_string(importResult.successfulImports));
        qDebug() << QString::fromUtf8("导入失败: " + std::to_string(importResult.failedImports));
        qDebug() << QString::fromUtf8("耗时: " + std::to_string(importResult.duration.count()) + "ms");
        
        if (!importResult.errors.empty()) {
            printWarning("导入过程中的错误:");
            for (const auto& error : importResult.errors) {
                qDebug() << QString::fromUtf8("  - " + error) +"\n";
            }
        }
        return true;
    } else {
        printError("文档数据导入失败: " + result.message);
        // 新增：失败时也打印所有错误
        if (result.data && !result.data->errors.empty()) {
            printWarning("导入过程中的错误:");
            for (const auto& error : result.data->errors) {
                qDebug() << QString::fromUtf8("  - " + error) +"\n";
            }
        }
        return false;
    }
}

bool CLIHandler::handleExportDocumentsExcel(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printError("用法: export-docs-excel <输出文件路径> [包含标题行]");
        printInfo("示例: export-docs-excel documents_export.xlsx true");
        return false;
    }
    
    std::string filePath = args[1];
    bool includeHeader = true;
    
    if (args.size() > 2) {
        includeHeader = (args[2] == "true" || args[2] == "1");
    }
    
    printInfo("开始导出文档数据...");
    printInfo("输出文件: " + filePath);
    printInfo("包含标题行: " + std::string(includeHeader ? "是" : "否"));
    
    auto result = importExportManager->exportDocumentsToCSV(filePath, includeHeader);
    
    if (result.success) {
        ExportResult exportResult = result.data.value();
        printSuccess("文档数据导出完成！");
        qDebug() << "导出记录数: " << exportResult.totalRecords +"\n";
        qDebug() << "文件大小: " << exportResult.fileSize << " bytes\n";
        qDebug() << "文件路径: " << exportResult.filePath +"\n";
        qDebug() << "耗时: " << exportResult.duration.count() << "ms\n";
        return true;
    } else {
        printError("文档数据导出失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleGenerateUserTemplate(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printError("用法: gen-user-template <输出文件路径>");
        printInfo("示例: gen-user-template user_template.xlsx");
        return false;
    }
    
    std::string filePath = args[1];
    
    printInfo("正在生成用户导入模板...");
    auto result = importExportManager->generateUserTemplate(filePath);
    
    if (result.success) {
        printSuccess("用户导入模板生成成功！");
        qDebug() << "模板文件: " << filePath +"\n";
        qDebug() << "模板包含以下列:\n";
        qDebug() << "  - 用户名 (必需)\n";
        qDebug() << "  - 邮箱 (必需)\n";
        qDebug() << "  - 密码 (可选，如不提供将使用默认密码)\n";
        qDebug() << "  - 是否激活 (可选，默认为1)\n";
        return true;
    } else {
        printError("用户导入模板生成失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleGenerateDocumentTemplate(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printError("用法: gen-doc-template <输出文件路径>");
        printInfo("示例: gen-doc-template document_template.xlsx");
        return false;
    }
    
    std::string filePath = args[1];
    
    printInfo("正在生成文档导入模板...");
    auto result = importExportManager->generateDocumentTemplate(filePath);
    
    if (result.success) {
        printSuccess("文档导入模板生成成功！");
        qDebug() << "模板文件: " << filePath +"\n";
        qDebug() << "模板包含以下列:\n";
        qDebug() << "  - 标题 (必需)\n";
        qDebug() << "  - 描述 (可选)\n";
        qDebug() << "  - 文件路径 (可选)\n";
        qDebug() << "  - 所有者ID (必需)\n";
        qDebug() << "  - 文件大小 (可选)\n";
        qDebug() << "  - 内容类型 (可选)\n";
        return true;
    } else {
        printError("文档导入模板生成失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handlePreviewExcel(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printError("用法: preview-excel <Excel文件路径> [最大预览行数]");
        printInfo("示例: preview-excel data.xlsx 10");
        return false;
    }
    
    std::string filePath = args[1];
    int maxRows = 10;
    
    if (args.size() > 2) {
        try {
            maxRows = std::stoi(args[2]);
        } catch (...) {
            printError("最大预览行数必须是数字");
            return false;
        }
    }
    
    if (!Utils::fileExists(filePath)) {
        printError("文件不存在: " + filePath);
        return false;
    }
    
    printInfo("正在预览Excel文件...");
    auto result = importExportManager->previewCSVData(filePath, maxRows);
    
    if (result.success) {
        DataPreview preview = result.data.value();
        printSuccess("Excel文件预览成功！");
        qDebug() << "总行数: " << preview.totalRows +"\n";
        qDebug() << "预览行数: " << preview.rows.size() +"\n";
        qDebug() << "列数: " << preview.headers.size() +"\n";
        
        if (!preview.headers.empty()) {
            qDebug() << "\n列标题:\n";
            for (size_t i = 0; i < preview.headers.size(); ++i) {
                qDebug() << "  " << (i + 1) << ". " << preview.headers[i] +"\n";
            }
        }
        
        if (!preview.rows.empty()) {
            qDebug() << "\n预览数据:\n";
            qDebug() << std::string(80, '-') +"\n";
            
            // 打印标题行

            qDebug() <<"\n";
            qDebug() << std::string(80, '-') +"\n";
            
            // 打印数据行
            for (const auto& row : preview.rows) {
                for (size_t i = 0; i < row.size() && i < preview.headers.size(); ++i) {
                    std::string cell = row[i];
                    if (cell.length() > 14) {
                        cell = cell.substr(0, 11) + "...";
                    }
       
                }
                qDebug() <<"\n";
            }
            qDebug() << std::string(80, '-') +"\n";
        }
        
        if (!preview.validationErrors.empty()) {
            printWarning("数据验证错误:");
            for (const auto& error : preview.validationErrors) {
                qDebug() << "  - " << error +"\n";
            }
        }
        
        return true;
    } else {
        printError("Excel文件预览失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleValidateExcel(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        printError("用法: validate-excel <Excel文件路径> <数据类型>");
        printInfo("数据类型: users 或 documents");
        printInfo("示例: validate-excel users.xlsx users");
        return false;
    }
    
    std::string filePath = args[1];
    std::string dataType = args[2];
    
    if (dataType != "users" && dataType != "documents") {
        printError("数据类型必须是 'users' 或 'documents'");
        return false;
    }
    
    if (!Utils::fileExists(filePath)) {
        printError("文件不存在: " + filePath);
        return false;
    }
    
    printInfo("正在验证Excel文件...");
    auto result = importExportManager->validateCSVFile(filePath, dataType);
    
    if (result.success) {
        printSuccess("Excel文件验证通过！");
        qDebug() << "文件: " << filePath +"\n";
        qDebug() << "数据类型: " << dataType +"\n";
        qDebug() << "验证结果: 通过\n";
        return true;
    } else {
        printError("Excel文件验证失败: " + result.message);
        return false;
    }
}

bool CLIHandler::handleExportCommandsCSV(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        printError("用法: export-commands-csv <输出文件路径>");
        printInfo("示例: export-commands-csv D://test//commands_export.csv");
        return false;
    }
    std::string filePath = args[1];
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        printError("无法创建文件: " + filePath);
        return false;
    }
    file << "\xEF\xBB\xBF"; // UTF-8 BOM
    file << "命令名,描述,用法,示例,需要登录,别名\n";
    std::set<std::string> exported; // 防止重复导出别名
    for (const auto& [name, cmd] : commands) {
        if (exported.count(name)) continue;
        exported.insert(name);
        file << '"' << name << '"' << ',';
        file << '"' << cmd.description << '"' << ',';
        file << '"' << cmd.usage << '"' << ',';
        if (!cmd.examples.empty()) file << '"' << cmd.examples[0] << '"';
        file << ',';
        file << (cmd.requiresAuth ? "是" : "否") << ',';
        if (!cmd.aliases.empty()) {
            for (size_t i = 0; i < cmd.aliases.size(); ++i) {
                if (i > 0) file << '|';
                file << cmd.aliases[i];
            }
        }
        file << "\n";
    }
    file.close();
    printSuccess("命令导出成功: " + filePath);
    return true;
}

bool CLIHandler::handleHistory(const std::vector<std::string>& args) {
    std::ifstream file("command_history.log");
    if (!file.is_open()) {
        printError("无法读取命令历史文件");
        return false;
    }
    std::string line;
    int idx = 1;
    qDebug() << "命令历史记录：\n";
    while (std::getline(file, line)) {
        qDebug() << idx++ << ": " << line +"\n";
    }
    return true;
}

void CLIHandler::completionCallback(const char* prefix, linenoiseCompletions* lc) {
    if (!instance) return;
    std::string pre(prefix);
    for (const auto& [cmd, _] : instance->commands) {
        if (cmd.find(pre) == 0) {
            linenoiseAddCompletion(lc, cmd.c_str());
        }
    }
}

std::vector<User> CLIHandler::getAllUsersForUI() {
    auto result = authManager->getAllUsers();
    if (result.success) {
        return result.data.value();
    }
    return {};
}

std::vector<User> CLIHandler::getSearchedUsersForUI(const std::string& keyword) {
    auto result = dbManager->searchUsers(keyword, 50);
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
    std::vector<Document> filtered;
    if (result.success) {
        for (const auto& doc : result.data.value()) {
            if (doc.owner_id == userId) {
                filtered.push_back(doc);
            }
        }
    }
    return filtered;
}

bool CLIHandler::handleExportDocsExcel(const std::string& username, const std::string& filePath) {
    // 1. 查找用户
    auto userResult = dbManager->getUserByUsername(username);
    if (!userResult.success) {
        printError("未找到用户: " + username);
        return false;
    }
    int userId = userResult.data->id;

    // 2. 获取该用户所有文档
    auto docsResult = dbManager->getDocumentsByOwner(userId, 1000, 0);
    if (!docsResult.success) {
        printError("获取用户文档失败: " + docsResult.message);
        return false;
    }
    const auto& docs = docsResult.data.value();

    // 3. 写入CSV文件
    std::ofstream file(filePath);
    if (!file.is_open()) {
        printError("无法创建文件: " + filePath);
        return false;
    }
    // 写表头
    file << "ID,标题,描述,文件名,Minio键,所有者ID,创建时间,更新时间,文件大小,文件后缀\n";
    for (const auto& doc : docs) {
        file << doc.id << ","
             << '"' << doc.title << '"' << ","
             << '"' << doc.description << '"' << ","
             << '"' << doc.file_path << '"' << ","
             << '"' << doc.minio_key << '"' << ","
             << doc.owner_id << ","
             << '"' << Utils::formatTimestamp(doc.created_at) << '"' << ","
             << '"' << Utils::formatTimestamp(doc.updated_at) << '"' << ","
             << doc.file_size << ","
             << '"' << doc.content_type << '"' << "\n";
    }
    file.close();
    printSuccess("文档导出成功: " + filePath);
    return true;
}
