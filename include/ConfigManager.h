#pragma once

#include "Common.h"
#include <mutex>
#include <memory>

class ConfigManager {
private:
    json config;
    static std::unique_ptr<ConfigManager> instance;
    static std::mutex mutex_;

    ConfigManager() = default;

public:
    static ConfigManager* getInstance();
    bool loadConfig(const std::string& configFile = "config.json");

    // Database configuration
    std::string getDatabaseType() const;
    std::string getSqliteFilename() const;
    std::string getMysqlHost() const;
    int getMysqlPort() const;
    std::string getMysqlUsername() const;
    std::string getMysqlPassword() const;
    std::string getMysqlDatabase() const;

    // Redis configuration
    std::string getRedisHost() const;
    int getRedisPort() const;
    std::string getRedisPassword() const;
    int getRedisDatabase() const;
    int getRedisTimeout() const;

    // MinIO configuration
    std::string getMinioEndpoint() const;
    std::string getMinioAccessKey() const;
    std::string getMinioSecretKey() const;
    std::string getMinioBucket() const;
    bool getMinioSecure() const;

    // Authentication configuration
    int getSessionTimeout() const;
    int getPasswordMinLength() const;
    int getMaxLoginAttempts() const;

    // Logging configuration
    std::string getLogLevel() const;
    std::string getLogFile() const;
    size_t getLogMaxSize() const;
    int getLogBackupCount() const;

    // Cache configuration
    int getCacheTTL() const;
    int getCacheMaxEntries() const;

    // Get raw config value
    json getConfig(const std::string& key) const;
};