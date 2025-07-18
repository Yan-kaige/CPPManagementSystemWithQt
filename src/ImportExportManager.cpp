#include "../include/ImportExportManager.h"
#include "../include/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <iomanip>

// CSV文件处理实现
// 支持标准的逗号分隔值格式，兼容Excel导出的CSV文件

ImportExportManager::ImportExportManager(DatabaseManager* db) 
    : dbManager(db), dateFormat("%Y-%m-%d %H:%M:%S"), encoding("UTF-8"), 
      skipEmptyRows(true), trimWhitespace(true) {
    if (!dbManager) {
        throw std::invalid_argument("DatabaseManager cannot be null");
    }
}

ImportExportManager::~ImportExportManager() = default;

// 用户数据导入导出
Result<ImportResult> ImportExportManager::importUsersFromCSV(const std::string& filePath, bool skipHeader) {
    auto startTime = std::chrono::high_resolution_clock::now();
    ImportResult result;
    
    try {
        if (!isCSVFile(filePath)) {
            return Result<ImportResult>::Error("文件不是有效的CSV格式");
        }
        
        if (!QFile::exists(QString::fromUtf8(filePath))) {

            return Result<ImportResult>::Error("文件不存在: " + filePath);
        }
        
        // 读取CSV文件内容
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return Result<ImportResult>::Error("无法打开文件: " + filePath);
        }
        
        std::string line;
        std::vector<std::string> headers;
        int lineNumber = 0;
        
        // 读取标题行
        if (std::getline(file, line)) {
            headers = Utils::splitString(line, ',');
            lineNumber++;
        }
        
        // 跳过标题行（如果需要）
        if (skipHeader && lineNumber == 1) {
            // 标题行已读取，继续处理数据行
        }
        
        // 在导入函数内部，headers读取后，建立映射表：
        std::map<std::string, std::string> headerMap = {
            {"用户名", "username"},
            {"邮箱", "email"},
            {"密码", "password"},
            {"是否激活", "is_active"},
            {"标题", "title"},
            {"描述", "description"},
            {"文件路径", "file_path"},
            {"所有者ID", "owner_id"},
            {"文件大小", "file_size"},
            {"内容类型", "content_type"}
        };
        
        // 处理数据行
        while (std::getline(file, line)) {
            lineNumber++;
            
            if (trimWhitespace) {
                line = Utils::trim(line);
            }
            
            if (skipEmptyRows && line.empty()) {
                continue;
            }
            
            result.totalRecords++;
            
            try {
                std::vector<std::string> values = Utils::splitString(line, ',');
                if (values.size() != headers.size()) {
                    std::string err = "第" + std::to_string(lineNumber) + "行: 列数不匹配";
                    addError(err);
                    result.errors.push_back(err);
                    result.failedImports++;
                    continue;
                }
                
                // 构建数据映射
                std::map<std::string, std::string> data;
                for (size_t i = 0; i < headers.size(); ++i) {
                    std::string key = headers[i];
                    if (headerMap.count(key)) {
                        data[headerMap[key]] = values[i];
                    } else {
                        data[key] = values[i]; // 保底
                    }
                }
                
//                 验证数据
                if (!validateUserData(data)) {
                    std::string err = "第" + std::to_string(lineNumber) + "行: 数据验证失败";
                    addError(err);
                    result.errors.push_back(err);
                    result.failedImports++;
                    continue;
                }

                // 转换为User对象并保存
                User user = mapToUser(data);
                auto dbResult = dbManager->createUser(user.username, user.password_hash, user.email);
                
                if (dbResult.success) {
                    result.successfulImports++;
                } else {
                    std::string err = "第" + std::to_string(lineNumber) + "行: " + dbResult.message;
                    addError(err);
                    result.errors.push_back(err);
                    result.failedImports++;
                }
                
                // 更新进度
                updateProgress(result.totalRecords, result.totalRecords, "正在导入用户数据...");
                
            } catch (const std::exception& e) {
                std::string err = "第" + std::to_string(lineNumber) + "行: " + std::string(e.what());
                addError(err);
                result.errors.push_back(err);
                result.failedImports++;
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        return Result<ImportResult>::Success(result, "导入完成");
        
    } catch (const std::exception& e) {
        return Result<ImportResult>::Error("导入失败: " + std::string(e.what()));
    }
}

Result<ExportResult> ImportExportManager::exportUsersToCSV(const std::string& filePath, bool includeHeader) {
    auto startTime = std::chrono::high_resolution_clock::now();
    ExportResult result;
    
    try {
        // 获取所有用户
        auto usersResult = dbManager->getAllUsers();
        if (!usersResult.success) {
            return Result<ExportResult>::Error("获取用户数据失败: " + usersResult.message);
        }
        
        std::ofstream file(filePath);
        if (!file.is_open()) {
            return Result<ExportResult>::Error("无法创建文件: " + filePath);
        }
        
        // 写入标题行
        if (includeHeader) {
            file << "ID,用户名,邮箱,创建时间,最后登录,是否激活\n";
        }
        
        // 写入数据行
        for (const auto& user : usersResult.data.value()) {
            auto userMap = userToMap(user);
            file << userMap["id"] << ","
                 << userMap["username"] << ","
                 << userMap["email"] << ","
                 << userMap["created_at"] << ","
                 << userMap["last_login"] << ","
                 << userMap["is_active"] << "\n";
            
            result.totalRecords++;
        }
        
        file.close();
        
        // 获取文件大小
        QFileInfo fileInfo(QString::fromUtf8(filePath));
        result.fileSize = fileInfo.size();
        result.filePath = filePath;
        
        auto endTime = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        return Result<ExportResult>::Success(result, "导出完成");
        
    } catch (const std::exception& e) {
        return Result<ExportResult>::Error("导出失败: " + std::string(e.what()));
    }
}

// 文档数据导入导出
Result<ImportResult> ImportExportManager::importDocumentsFromCSV(const std::string& filePath, bool skipHeader) {
    auto startTime = std::chrono::high_resolution_clock::now();
    ImportResult result;
    
    try {
        if (!isCSVFile(filePath)) {
            return Result<ImportResult>::Error("文件不是有效的CSV格式");
        }
        
        if (!QFile::exists(QString::fromUtf8(filePath))) {

            return Result<ImportResult>::Error("文件不存在: " + filePath);
        }
        
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return Result<ImportResult>::Error("无法打开文件: " + filePath);
        }
        
        std::string line;
        std::vector<std::string> headers;
        int lineNumber = 0;
        
        // 读取标题行
        if (std::getline(file, line)) {
            headers = Utils::splitString(line, ',');
            lineNumber++;
        }
        
        // 处理数据行
        while (std::getline(file, line)) {
            lineNumber++;
            
            if (trimWhitespace) {
                line = Utils::trim(line);
            }
            
            if (skipEmptyRows && line.empty()) {
                continue;
            }
            
            result.totalRecords++;
            
            try {
                std::vector<std::string> values = Utils::splitString(line, ',');
                if (values.size() != headers.size()) {
                    std::string err = "第" + std::to_string(lineNumber) + "行: 列数不匹配";
                    addError(err);
                    result.errors.push_back(err);
                    result.failedImports++;
                    continue;
                }
                
                // 构建数据映射
                std::map<std::string, std::string> data;
                for (size_t i = 0; i < headers.size(); ++i) {
                    data[headers[i]] = values[i];
                }
                
                // 验证数据
                if (!validateDocumentData(data)) {
                    std::string err = "第" + std::to_string(lineNumber) + "行: 数据验证失败";
                    addError(err);
                    result.errors.push_back(err);
                    result.failedImports++;
                    continue;
                }
                
                // 转换为Document对象并保存
                Document doc = mapToDocument(data);
                auto dbResult = dbManager->createDocument(doc.title, doc.description, doc.file_path, 
                                                        doc.minio_key, doc.owner_id, doc.file_size, doc.content_type);
                
                if (dbResult.success) {
                    result.successfulImports++;
                } else {
                    std::string err = "第" + std::to_string(lineNumber) + "行: " + dbResult.message;
                    addError(err);
                    result.errors.push_back(err);
                    result.failedImports++;
                }
                
                updateProgress(result.totalRecords, result.totalRecords, "正在导入文档数据...");
                
            } catch (const std::exception& e) {
                std::string err = "第" + std::to_string(lineNumber) + "行: " + std::string(e.what());
                addError(err);
                result.errors.push_back(err);
                result.failedImports++;
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        return Result<ImportResult>::Success(result, "导入完成");
        
    } catch (const std::exception& e) {
        return Result<ImportResult>::Error("导入失败: " + std::string(e.what()));
    }
}

Result<ExportResult> ImportExportManager::exportDocumentsToCSV(const std::string& filePath, bool includeHeader) {
    auto startTime = std::chrono::high_resolution_clock::now();
    ExportResult result;
    
    try {
        // 获取所有文档
        auto docsResult = dbManager->getAllDocuments();
        if (!docsResult.success) {
            return Result<ExportResult>::Error("获取文档数据失败: " + docsResult.message);
        }
        
        std::ofstream file(filePath);
        if (!file.is_open()) {
            return Result<ExportResult>::Error("无法创建文件: " + filePath);
        }
        
        // 写入标题行
        if (includeHeader) {
            file << "ID,标题,描述,文件名,Minio键,所有者ID,创建时间,更新时间,文件大小,文件后缀\n";
        }
        
        // 写入数据行
        for (const auto& doc : docsResult.data.value()) {
            auto docMap = documentToMap(doc);
            file << docMap["id"] << ","
                 << docMap["title"] << ","
                 << docMap["description"] << ","
                 << docMap["file_path"] << ","
                 << docMap["minio_key"] << ","
                 << docMap["owner_id"] << ","
                 << docMap["created_at"] << ","
                 << docMap["updated_at"] << ","
                 << docMap["file_size"] << ","
                 << docMap["content_type"] << "\n";
            
            result.totalRecords++;
        }
        
        file.close();
        
        QFileInfo fileInfo(QString::fromUtf8(filePath));
        result.fileSize = fileInfo.size();
        result.filePath = filePath;
        
        auto endTime = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        return Result<ExportResult>::Success(result, "导出完成");
        
    } catch (const std::exception& e) {
        return Result<ExportResult>::Error("导出失败: " + std::string(e.what()));
    }
}

// 数据预览和验证
Result<DataPreview> ImportExportManager::previewCSVData(const std::string& filePath, int maxRows) {
    DataPreview preview;
    try {
        if (!isCSVFile(filePath)) {
            return Result<DataPreview>::Error("文件不是有效的CSV格式");
        }
        if (!QFile::exists(QString::fromUtf8(filePath))) {

            return Result<DataPreview>::Error("文件不存在: " + filePath);
        }
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return Result<DataPreview>::Error("无法打开文件: " + filePath);
        }
        std::string line;
        int lineNumber = 0;
        // 读取标题行
        if (std::getline(file, line)) {
            preview.headers = Utils::splitString(line, ',');
            lineNumber++;
        }
        // 读取预览行
        while (std::getline(file, line) && lineNumber <= maxRows) {
            if (this->trimWhitespace) {
                line = Utils::trim(line);
            }
            if (this->skipEmptyRows && line.empty()) {
                continue;
            }
            std::vector<std::string> values = Utils::splitString(line, ',');
            preview.rows.push_back(values);
            lineNumber++;
        }
        // 计算总行数
        file.clear();
        file.seekg(0);
        std::getline(file, line); // 跳过标题行
        int totalRows = 0;
        while (std::getline(file, line)) {
            if (!this->skipEmptyRows || !line.empty()) {
                totalRows++;
            }
        }
        preview.totalRows = totalRows;
        preview.isValid = true;
        return Result<DataPreview>::Success(preview, "预览成功");
    } catch (const std::exception& e) {
        return Result<DataPreview>::Error("预览失败: " + std::string(e.what()));
    }
}

Result<bool> ImportExportManager::validateCSVFile(const std::string& filePath, const std::string& dataType) {
    try {
        auto previewResult = previewCSVData(filePath, 5);
        if (!previewResult.success) {
            return Result<bool>::Error(previewResult.message);
        }
        DataPreview preview = previewResult.data.value();
        // 根据数据类型验证必需的列
        if (dataType == "users") {
            std::vector<std::string> requiredColumns = {"username", "email"};
            for (const auto& col : requiredColumns) {
                if (std::find(preview.headers.begin(), preview.headers.end(), col) == preview.headers.end()) {
                    return Result<bool>::Error("缺少必需的列: " + col);
                }
            }
        } else if (dataType == "documents") {
            std::vector<std::string> requiredColumns = {"title", "owner_id"};
            for (const auto& col : requiredColumns) {
                if (std::find(preview.headers.begin(), preview.headers.end(), col) == preview.headers.end()) {
                    return Result<bool>::Error("缺少必需的列: " + col);
                }
            }
        }
        return Result<bool>::Success(true, "文件验证通过");
    } catch (const std::exception& e) {
        return Result<bool>::Error("验证失败: " + std::string(e.what()));
    }
}

// 模板生成
Result<bool> ImportExportManager::generateUserTemplate(const std::string& filePath) {
    try {
        std::ofstream file(filePath);
        if (!file.is_open()) {
            return Result<bool>::Error("无法创建文件: " + filePath);
        }
        
        file << "用户名,邮箱,密码,是否激活\n";
        file << "admin,admin@example.com,password123,1\n";
        file << "user1,user1@example.com,password123,1\n";
        
        file.close();
        return Result<bool>::Success(true, "用户模板生成成功");
        
    } catch (const std::exception& e) {
        return Result<bool>::Error("模板生成失败: " + std::string(e.what()));
    }
}

Result<bool> ImportExportManager::generateDocumentTemplate(const std::string& filePath) {
    try {
        std::ofstream file(filePath);
        if (!file.is_open()) {
            return Result<bool>::Error("无法创建文件: " + filePath);
        }
        
        file << "标题,描述,文件名,所有者ID,文件大小,文件后缀\n";
        file << "示例文档,这是一个示例文档,file.pdf,1,1024,pdf\n";
        file << "测试文档,测试文档描述,test.txt,1,512,txt\n";
        
        file.close();
        return Result<bool>::Success(true, "文档模板生成成功");
        
    } catch (const std::exception& e) {
        return Result<bool>::Error("模板生成失败: " + std::string(e.what()));
    }
}

// 配置设置
void ImportExportManager::setDateFormat(const std::string& format) {
    dateFormat = format;
}

void ImportExportManager::setEncoding(const std::string& enc) {
    encoding = enc;
}

void ImportExportManager::setSkipEmptyRows(bool skip) {
    skipEmptyRows = skip;
}

void ImportExportManager::setTrimWhitespace(bool trim) {
    trimWhitespace = trim;
}

// 进度回调设置
void ImportExportManager::setProgressCallback(ProgressCallback callback) {
    progressCallback = callback;
}

void ImportExportManager::clearProgressCallback() {
    progressCallback = nullptr;
}

// 错误处理
std::vector<std::string> ImportExportManager::getLastErrors() const {
    return lastErrors;
}

void ImportExportManager::clearLastErrors() {
    lastErrors.clear();
}

// 工具函数
std::vector<std::string> ImportExportManager::getSupportedFormats() {
    return {".csv"};
}

bool ImportExportManager::isCSVFile(const std::string& filePath) {
    std::string ext = ImportExportManager::getFileExtension(filePath);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".csv";
}

// 私有辅助函数实现
bool ImportExportManager::isValidCSVFile(const std::string& filePath) {
    return isCSVFile(filePath) && QFile::exists(QString::fromUtf8(filePath));
}


std::string ImportExportManager::getFileExtension(const std::string& filePath) {
    size_t pos = filePath.find_last_of('.');
    if (pos != std::string::npos) {
        return filePath.substr(pos);
    }
    return "";
}

bool ImportExportManager::validateUserData(const std::map<std::string, std::string>& data) {
    // 检查必需字段
    if (data.find("username") == data.end() || data.at("username").empty()) {
        return false;
    }
    
    if (data.find("email") == data.end() || data.at("email").empty()) {
        return false;
    }
    
    // 验证邮箱格式
    if (!Utils::isValidEmail(data.at("email"))) {
        return false;
    }
    
    return true;
}

bool ImportExportManager::validateDocumentData(const std::map<std::string, std::string>& data) {
    // 检查必需字段
    if (data.find("title") == data.end() || data.at("title").empty()) {
        return false;
    }
    
    if (data.find("owner_id") == data.end()) {
        return false;
    }
    
    // 验证owner_id是否为数字
    try {
        std::stoi(data.at("owner_id"));
    } catch (...) {
        return false;
    }
    
    return true;
}

User ImportExportManager::mapToUser(const std::map<std::string, std::string>& data) {
    User user;
    user.id = 0; // 数据库自动生成
    user.username = data.at("username");
    user.email = data.at("email");
    user.password_hash = data.count("password") ? Utils::sha256Hash(data.at("password")) : "";
    user.created_at = std::chrono::system_clock::now();
    user.last_login = std::chrono::system_clock::now();
    user.is_active = data.count("is_active") ? (data.at("is_active") == "1" || data.at("is_active") == "true") : true;
    
    return user;
}

Document ImportExportManager::mapToDocument(const std::map<std::string, std::string>& data) {
    Document doc;
    doc.id = 0; // 数据库自动生成
    doc.title = data.at("title");
    doc.description = data.count("description") ? data.at("description") : "";
    doc.file_path = data.count("file_path") ? data.at("file_path") : "";
    doc.minio_key = data.count("minio_key") ? data.at("minio_key") : "";
    doc.owner_id = std::stoi(data.at("owner_id"));
    doc.created_at = std::chrono::system_clock::now();
    doc.updated_at = std::chrono::system_clock::now();
    doc.file_size = data.count("file_size") ? std::stoul(data.at("file_size")) : 0;
    doc.content_type = data.count("content_type") ? data.at("content_type") : "application/octet-stream";
    
    return doc;
}

std::map<std::string, std::string> ImportExportManager::userToMap(const User& user) {
    std::map<std::string, std::string> data;
    data["id"] = std::to_string(user.id);
    data["username"] = user.username;
    data["email"] = user.email;
    data["created_at"] = Utils::formatTimestamp(user.created_at);
    data["last_login"] = Utils::formatTimestamp(user.last_login);
    data["is_active"] = user.is_active ? "1" : "0";
    
    return data;
}

std::map<std::string, std::string> ImportExportManager::documentToMap(const Document& doc) {
    std::map<std::string, std::string> data;
    data["id"] = std::to_string(doc.id);
    data["title"] = doc.title;
    data["description"] = doc.description;
    data["file_path"] = doc.file_path;
    data["minio_key"] = doc.minio_key;
    data["owner_id"] = std::to_string(doc.owner_id);
    data["created_at"] = Utils::formatTimestamp(doc.created_at);
    data["updated_at"] = Utils::formatTimestamp(doc.updated_at);
    data["file_size"] = std::to_string(doc.file_size);
    data["content_type"] = doc.content_type;
    
    return data;
}

void ImportExportManager::updateProgress(int current, int total, const std::string& status) {
    if (progressCallback) {
        progressCallback(current, total, status);
    }
}

void ImportExportManager::addError(const std::string& error) {
    lastErrors.push_back(error);
}

void ImportExportManager::clearErrors() {
    lastErrors.clear();
}
