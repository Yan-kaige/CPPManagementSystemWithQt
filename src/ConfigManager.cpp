#include "ConfigManager.h"
#include <fstream>

std::unique_ptr<ConfigManager> ConfigManager::instance = nullptr;
std::mutex ConfigManager::mutex_;

ConfigManager* ConfigManager::getInstance() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance == nullptr) {
        instance = std::unique_ptr<ConfigManager>(new ConfigManager());
    }
    return instance.get();
}

bool ConfigManager::loadConfig(const std::string& configFile) {
    try {
        std::ifstream file(configFile);
        if (!file.is_open()) {
            std::cerr << "无法打开配置文件: " << configFile << std::endl;
            return false;
        }

        file >> config;
        file.close();

        std::cout << "配置文件加载成功: " << configFile << std::endl;
        return true;

    } catch (const json::exception& e) {
        std::cerr << "配置文件解析错误: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "加载配置文件时发生错误: " << e.what() << std::endl;
        return false;
    }
}

// 数据库配置
std::string ConfigManager::getDatabaseType() const {
    return config.value("database", json::object()).value("type", "sqlite");
}

std::string ConfigManager::getSqliteFilename() const {
    return config.value("database", json::object())
            .value("sqlite", json::object())
            .value("filename", "data/management.db");
}

std::string ConfigManager::getMysqlHost() const {
    return config.value("database", json::object())
            .value("mysql", json::object())
            .value("host", "localhost");
}

int ConfigManager::getMysqlPort() const {
    return config.value("database", json::object())
            .value("mysql", json::object())
            .value("port", 3306);
}

std::string ConfigManager::getMysqlUsername() const {
    return config.value("database", json::object())
            .value("mysql", json::object())
            .value("username", "root");
}

std::string ConfigManager::getMysqlPassword() const {
    return config.value("database", json::object())
            .value("mysql", json::object())
            .value("password", "");
}

std::string ConfigManager::getMysqlDatabase() const {
    return config.value("database", json::object())
            .value("mysql", json::object())
            .value("database", "management_system");
}

// Redis配置
std::string ConfigManager::getRedisHost() const {
    return config.value("redis", json::object()).value("host", "127.0.0.1");
}

int ConfigManager::getRedisPort() const {
    return config.value("redis", json::object()).value("port", 6379);
}

std::string ConfigManager::getRedisPassword() const {
    return config.value("redis", json::object()).value("password", "");
}

int ConfigManager::getRedisDatabase() const {
    return config.value("redis", json::object()).value("database", 0);
}

int ConfigManager::getRedisTimeout() const {
    return config.value("redis", json::object()).value("timeout", 5000);
}

// MinIO配置
std::string ConfigManager::getMinioEndpoint() const {
    return config.value("minio", json::object()).value("endpoint", "127.0.0.1:9000");
}

std::string ConfigManager::getMinioAccessKey() const {
    return config.value("minio", json::object()).value("access_key", "minioadmin");
}

std::string ConfigManager::getMinioSecretKey() const {
    return config.value("minio", json::object()).value("secret_key", "minioadmin");
}

std::string ConfigManager::getMinioBucket() const {
    return config.value("minio", json::object()).value("bucket", "management-files");
}

bool ConfigManager::getMinioSecure() const {
    return config.value("minio", json::object()).value("secure", false);
}

// 认证配置
int ConfigManager::getSessionTimeout() const {
    return config.value("auth", json::object()).value("session_timeout", 3600);
}

int ConfigManager::getPasswordMinLength() const {
    return config.value("auth", json::object()).value("password_min_length", 6);
}

int ConfigManager::getMaxLoginAttempts() const {
    return config.value("auth", json::object()).value("max_login_attempts", 5);
}

// 日志配置
std::string ConfigManager::getLogLevel() const {
    return config.value("logging", json::object()).value("level", "INFO");
}

std::string ConfigManager::getLogFile() const {
    return config.value("logging", json::object()).value("file", "logs/system.log");
}

size_t ConfigManager::getLogMaxSize() const {
    return config.value("logging", json::object()).value("max_size", 10485760); // 10MB
}

int ConfigManager::getLogBackupCount() const {
    return config.value("logging", json::object()).value("backup_count", 5);
}

// 缓存配置
int ConfigManager::getCacheTTL() const {
    return config.value("cache", json::object()).value("ttl", 1800);
}

int ConfigManager::getCacheMaxEntries() const {
    return config.value("cache", json::object()).value("max_entries", 1000);
}

// 获取原始配置值
json ConfigManager::getConfig(const std::string& key) const {
    try {
        return config.at(key);
    } catch (const json::out_of_range& e) {
        return json::object();
    }
}