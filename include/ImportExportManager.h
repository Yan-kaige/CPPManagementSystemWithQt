#pragma once

#include "Common.h"
#include "DatabaseManager.h"
#include <functional>
#include <memory>

// CSV文件格式枚举
enum class CSVFormat {
    CSV     // 逗号分隔值格式 (.csv)
};

// 导入结果结构
struct ImportResult {
    int totalRecords;
    int successfulImports;
    int failedImports;
    std::vector<std::string> errors;
    std::chrono::milliseconds duration;
    
    ImportResult() : totalRecords(0), successfulImports(0), failedImports(0), duration(0) {}
};

// 导出结果结构
struct ExportResult {
    int totalRecords;
    size_t fileSize;
    std::string filePath;
    std::chrono::milliseconds duration;
    
    ExportResult() : totalRecords(0), fileSize(0), duration(0) {}
};

// 数据预览结构
struct DataPreview {
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
    int totalRows;
    bool isValid;
    std::vector<std::string> validationErrors;
};

// 进度回调函数类型
using ProgressCallback = std::function<void(int current, int total, const std::string& status)>;

class ImportExportManager {
private:
    DatabaseManager* dbManager;
    
    // 配置选项
    std::string dateFormat;
    std::string encoding;
    bool skipEmptyRows;
    bool trimWhitespace;
    
    // 进度回调
    ProgressCallback progressCallback;
    
    // 错误记录
    std::vector<std::string> lastErrors;
    
    // CSV处理辅助函数
    bool isValidCSVFile(const std::string& filePath);
    static std::string getFileExtension(const std::string& filePath);
    
    // 数据验证函数
    bool validateUserData(const std::map<std::string, std::string>& data);
    bool validateDocumentData(const std::map<std::string, std::string>& data);
    
    // 数据转换函数
    User mapToUser(const std::map<std::string, std::string>& data);
    Document mapToDocument(const std::map<std::string, std::string>& data);
    std::map<std::string, std::string> userToMap(const User& user);
    std::map<std::string, std::string> documentToMap(const Document& doc);
    
    // 进度更新
    void updateProgress(int current, int total, const std::string& status);
    
    // 错误处理
    void addError(const std::string& error);
    void clearErrors();

public:
    ImportExportManager(DatabaseManager* db);
    ~ImportExportManager();
    
    // 用户数据导入导出
    Result<ImportResult> importUsersFromCSV(const std::string& filePath, bool skipHeader = true);
    Result<ExportResult> exportUsersToCSV(const std::string& filePath, bool includeHeader = true);
    
    // 文档数据导入导出
    Result<ImportResult> importDocumentsFromCSV(const std::string& filePath, bool skipHeader = true);
    Result<ExportResult> exportDocumentsToCSV(const std::string& filePath, bool includeHeader = true);
    
    // 数据预览和验证
    Result<DataPreview> previewCSVData(const std::string& filePath, int maxRows = 10);
    Result<bool> validateCSVFile(const std::string& filePath, const std::string& dataType);
    
    // 模板生成
    Result<bool> generateUserTemplate(const std::string& filePath);
    Result<bool> generateDocumentTemplate(const std::string& filePath);
    
    // 配置设置
    void setDateFormat(const std::string& format);
    void setEncoding(const std::string& encoding);
    void setSkipEmptyRows(bool skip);
    void setTrimWhitespace(bool trim);
    
    // 进度回调设置
    void setProgressCallback(ProgressCallback callback);
    void clearProgressCallback();
    
    // 错误处理
    std::vector<std::string> getLastErrors() const;
    void clearLastErrors();
    
    // 工具函数
    static std::vector<std::string> getSupportedFormats();
    static bool isCSVFile(const std::string& filePath);
};