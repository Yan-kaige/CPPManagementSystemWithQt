#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <regex>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <random>

// JSON library - assuming nlohmann/json is available
#include <nlohmann/json.hpp>

// OpenSSL for hashing
#include <openssl/sha.h>
#include <openssl/evp.h>

// cURL for HTTP requests
#include <curl/curl.h>

// SQLite
#include <sqlite3.h>

using json = nlohmann::json;

// Common data structures
struct User {
    int id;
    std::string username;
    std::string password_hash;
    std::string email;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_login;
    bool is_active;
};

struct Document {
    int id;
    std::string title;
    std::string description;
    std::string file_path;
    std::string minio_key;
    int owner_id;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    size_t file_size;
    std::string content_type;
};

struct DocumentShare {
    int id;
    int document_id;
    int shared_by_user_id;
    int shared_to_user_id;
    int shared_document_id;
    std::string shared_minio_key;
    std::chrono::system_clock::time_point created_at;
};

// 权限管理相关结构体
enum class MenuType {
    DIRECTORY,  // 目录
    MENU,       // 菜单页面
    BUTTON      // 按钮权限
};

enum class ButtonType {
    ADD,
    EDIT,
    DELETE_BTN,  // 避免与关键字冲突
    VIEW,
    EXPORT,
    IMPORT,
    CUSTOM
};

struct Role {
    int id;
    std::string role_name;
    std::string role_code;
    std::string description;
    bool is_active;
    bool is_system;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    int created_by;
    int updated_by;
};

struct MenuItem {
    int id;
    std::string name;
    std::string code;
    int parent_id;
    MenuType type;
    std::string url;
    std::string icon;
    std::string permission_key;
    ButtonType button_type;
    int sort_order;
    bool is_visible;
    bool is_active;
    std::string description;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    int created_by;
    int updated_by;
};

struct UserRole {
    int id;
    int user_id;
    int role_id;
    std::chrono::system_clock::time_point assigned_at;
    int assigned_by;
    bool is_active;
    std::chrono::system_clock::time_point expires_at;
};

struct RoleMenu {
    int id;
    int role_id;
    int menu_id;
    bool is_granted;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    int created_by;
};

struct Session {
    int userId;
    std::string username;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point expiresAt;
};

// Response structure for operations
template<typename T>
struct Result {
    bool success;
    std::string message;
    std::optional<T> data;

    Result(bool s, const std::string& msg, std::optional<T> d = std::nullopt)
            : success(s), message(msg), data(d) {}

    static Result<T> Success(const T& data, const std::string& msg = "Success") {
        return Result<T>(true, msg, data);
    }

    static Result<T> Error(const std::string& msg) {
        return Result<T>(false, msg, std::nullopt);
    }

    static Result<T> Success(const std::string& msg = "Success") {
        return Result<T>(true, msg, std::nullopt);
    }
};

// Utility functions
class Utils {
public:
    static std::string generateRandomString(size_t length);
    static std::string sha256Hash(const std::string& input);
    static std::string getCurrentTimestamp();
    static std::chrono::system_clock::time_point parseTimestamp(const std::string& timestamp);
    static std::string formatTimestamp(const std::chrono::system_clock::time_point& tp);
    static std::vector<std::string> splitString(const std::string& str, char delimiter);
    static std::string toLower(const std::string& str);
    static std::string trim(const std::string& str);
    static bool isValidEmail(const std::string& email);
    static bool fileExists(const std::string& filename);
    static std::string getFilename(const std::string& path);
    static    std::string getFileExtension(const std::string& path);
    static std::string guessContentTypeByExtension(const std::string& extension);
};
