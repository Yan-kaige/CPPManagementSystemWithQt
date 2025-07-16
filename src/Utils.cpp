#include "Common.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <QFile>       // ✅ 新增
#include <QString>     // ✅ 如果未包含的话
#include <regex>

namespace {
    // 用于生成随机字符串的字符集
    const std::string CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    
    // 用于生成随机数的设备
    std::random_device rd;
    std::mt19937 gen(rd());
}

std::string Utils::generateRandomString(size_t length) {
    std::uniform_int_distribution<> dis(0, CHARS.size() - 1);
    std::string result;
    result.reserve(length);
    
    for (size_t i = 0; i < length; ++i) {
        result += CHARS[dis(gen)];
    }
    
    return result;
}

std::string Utils::sha256Hash(const std::string& input) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (context == nullptr) {
        return "";
    }
    
    if (EVP_DigestInit_ex(context, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(context);
        return "";
    }
    
    if (EVP_DigestUpdate(context, input.c_str(), input.length()) != 1) {
        EVP_MD_CTX_free(context);
        return "";
    }
    
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;
    
    if (EVP_DigestFinal_ex(context, hash, &lengthOfHash) != 1) {
        EVP_MD_CTX_free(context);
        return "";
    }
    
    EVP_MD_CTX_free(context);
    
    std::stringstream ss;
    for (unsigned int i = 0; i < lengthOfHash; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

std::string Utils::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    return formatTimestamp(now);
}

std::chrono::system_clock::time_point Utils::parseTimestamp(const std::string& timestamp) {
    std::tm tm = {};
    std::istringstream ss(timestamp);
    
    // 尝试解析 ISO 8601 格式: YYYY-MM-DD HH:MM:SS
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    
    if (ss.fail()) {
        // 如果失败，尝试其他格式
        ss.clear();
        ss.str(timestamp);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    }
    
    if (ss.fail()) {
        // 如果还是失败，返回当前时间
        return std::chrono::system_clock::now();
    }
    
    auto time_t = std::mktime(&tm);
    return std::chrono::system_clock::from_time_t(time_t);
}

std::string Utils::formatTimestamp(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::vector<std::string> Utils::splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string Utils::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string Utils::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(start, end - start + 1);
}

bool Utils::isValidEmail(const std::string& email) {
    // 基本的邮箱格式验证正则表达式
    const std::regex emailRegex(
        R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})"
    );
    
    return std::regex_match(email, emailRegex);
}
bool Utils::fileExists(const std::string& filename) {
    return QFile::exists(QString::fromUtf8(filename));
}


std::string Utils::getFilename(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

std::string Utils::getFileExtension(const std::string& path) {
    size_t pos = path.find_last_of('.');
    return (pos != std::string::npos) ? path.substr(pos) : "";
}

std::string Utils::guessContentTypeByExtension(const std::string& extension) {
    std::string ext = Utils::toLower(extension);
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".doc" || ext == ".docx") return "application/msword";
    if (ext == ".txt") return "text/plain";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png") return "image/png";
    if (ext == ".gif") return "image/gif";
    return "application/octet-stream"; // 默认
}
