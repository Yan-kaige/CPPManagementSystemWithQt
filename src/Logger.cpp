#include "Logger.h"
#include <iostream>
#include <ctime>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QDebug>

std::unique_ptr<Logger> Logger::instance = nullptr;
std::mutex Logger::mutex_;

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

Logger* Logger::getInstance() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!instance) {
        instance = std::unique_ptr<Logger>(new Logger());
    }
    return instance.get();
}


bool Logger::initialize(const std::string& filePath, LogLevel level, size_t maxSize, int backups) {
    logFilePath = filePath;
    currentLevel = level;
    maxFileSize = maxSize;
    backupCount = backups;
    currentSize = 0;

    QFileInfo fileInfo(QString::fromUtf8(filePath));
    QDir logDir = fileInfo.absoluteDir();
    if (!logDir.exists()) {
        logDir.mkpath(".");
    }

    logFile.open(filePath, std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "无法打开日志文件: " << filePath << std::endl;
        return false;
    }

    // 获取已有文件大小
    logFile.seekp(0, std::ios::end);
    currentSize = logFile.tellp();

    return true;
}

void Logger::rotateLogFile() {
    logFile.close();
    for (int i = backupCount - 1; i >= 0; --i) {
        QString oldName = QString::fromUtf8(logFilePath) + "." + QString::number(i);
        QString newName = QString::fromUtf8(logFilePath) + "." + QString::number(i + 1);

        if (QFile::exists(oldName)) {
            QFile::rename(oldName, newName);
        }
    }
    QFile::rename(QString::fromUtf8(logFilePath), QString::fromUtf8(logFilePath) + ".0");

    logFile.open(logFilePath, std::ios::trunc);
    currentSize = 0;
}
std::string Logger::logLevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERR: return "ERROR";
        default: return "UNKNOWN";
    }
}

LogLevel Logger::stringToLogLevel(const std::string& level) const {
    if (level == "DEBUG") return LogLevel::DEBUG;
    if (level == "INFO") return LogLevel::INFO;
    if (level == "WARNING") return LogLevel::WARNING;
    if (level == "ERROR") return LogLevel::ERR;
    return LogLevel::INFO;  // 默认
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < currentLevel) return;

    std::time_t now = std::time(nullptr);
    char timeBuf[20];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

    std::string levelStr = logLevelToString(level);
    std::string output = "[" + std::string(timeBuf) + "][" + levelStr + "] " + message + "\n";

    qDebug() << QString::fromUtf8(output);

    if (logFile.is_open()) {
        logFile << output;
        currentSize += output.size();
        if (currentSize > maxFileSize) {
            rotateLogFile();
        }
    }
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERR, message);
}

void Logger::setLevel(LogLevel level) {
    currentLevel = level;
}

LogLevel Logger::getLevel() const {
    return currentLevel;
}
