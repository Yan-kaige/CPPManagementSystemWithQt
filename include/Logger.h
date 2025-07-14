#pragma once

#include "Common.h"

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERR
};

class Logger {
private:
    static std::unique_ptr<Logger> instance;
    static std::mutex mutex_;

    std::ofstream logFile;
    LogLevel currentLevel;
    std::string logFilePath;
    size_t maxFileSize;
    int backupCount;
    size_t currentSize;

    Logger() = default;
    void rotateLogFile();
    std::string logLevelToString(LogLevel level) const;
    LogLevel stringToLogLevel(const std::string& level) const;

public:
    static Logger* getInstance();
    bool initialize(const std::string& filePath, LogLevel level, size_t maxSize, int backups);

    void log(LogLevel level, const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);

    void setLevel(LogLevel level);
    LogLevel getLevel() const;

    ~Logger();
};

// Convenience macros
#define LOG_DEBUG(msg) Logger::getInstance()->debug(msg)
#define LOG_INFO(msg) Logger::getInstance()->info(msg)
#define LOG_WARNING(msg) Logger::getInstance()->warning(msg)
#define LOG_ERROR(msg) Logger::getInstance()->error(msg)