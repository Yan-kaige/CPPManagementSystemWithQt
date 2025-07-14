#pragma once

#include "Common.h"
#include <mysql/mysql.h>
#include <mutex>

class DatabaseManager {
private:
    MYSQL* db;
    std::recursive_mutex dbMutex;
    bool isConnected;

    bool createTables();
    std::string getLastError() const;

public:
    DatabaseManager();
    ~DatabaseManager();

    bool connect(const std::string& host, int port, const std::string& username, 
                 const std::string& password, const std::string& database);
    void disconnect();
    bool isConnectionValid() const;

    // User operations
    Result<User> createUser(const std::string& username, const std::string& passwordHash,
                            const std::string& email);
    Result<User> getUserById(int userId);
    Result<User> getUserByUsername(const std::string& username);
    Result<std::vector<User>> getAllUsers(int limit = 100, int offset = 0);
    Result<bool> updateUser(const User& user);
    Result<bool> deleteUser(int userId);
    Result<bool> updateUserLastLogin(int userId);

    // Document operations
    Result<Document> createDocument(const std::string& title, const std::string& description,
                                    const std::string& filePath, const std::string& minioKey,
                                    int ownerId, size_t fileSize, const std::string& contentType);

    Result<Document> getDocumentById(int docId);
    Result<std::vector<Document>> getDocumentsByOwner(int ownerId, int limit = 100, int offset = 0);
    Result<std::vector<Document>> getAllDocuments(int limit = 100, int offset = 0);
    Result<bool> updateDocument(const Document& doc);
    Result<bool> deleteDocument(int docId);

    // Search operations
    Result<std::vector<User>> searchUsers(const std::string& query, int limit = 50);
    Result<std::vector<Document>> searchDocuments(const std::string& query, int limit = 50);

    // Statistics
    Result<int> getUserCount();
    Result<int> getDocumentCount();
    Result<size_t> getTotalFileSize();

    // Transaction support
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // Utility
    Result<bool> vacuum();
    Result<std::vector<std::map<std::string, std::string>>> executeQuery(const std::string& query);
};