#include "RedisManager.h"
#include <iostream>
#include <sstream>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

RedisManager::RedisManager()
        : host("127.0.0.1"), port(6379), password(""), database(0), timeout(5000),
          sockfd(-1), connected(false) {
#ifdef _WIN32
    // Windows下初始化Socket库
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

RedisManager::~RedisManager() {
    // 析构时断开连接并清理Socket库
    disconnect();
#ifdef _WIN32
    WSACleanup();
#endif
}

// 连接Redis服务器，支持指定host、port、密码、数据库、超时时间
bool RedisManager::connect(const std::string& host, int port, const std::string& password,
                           int database, int timeout) {
    this->host = host;
    this->port = port;
    this->password = password;
    this->database = database;
    this->timeout = timeout;
    
    return connectSocket();
}

// 实际建立Socket连接，并进行认证和数据库选择
bool RedisManager::connectSocket() {
    std::lock_guard<std::mutex> lock(redisMutex);
    
    // 创建socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return false;
    }
    
    // 设置为非阻塞模式，便于后续select超时处理
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sockfd, FIONBIO, &mode);
#else
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
#endif
    
    // 配置服务器地址结构体
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host.c_str());
    
    // 发起连接请求
    int result = ::connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (result < 0) {
#ifdef _WIN32
        // Windows下判断是否为非阻塞连接中的正常情况
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            closesocket(sockfd);
            return false;
        }
#else
        // Linux下判断是否为非阻塞连接中的正常情况
        if (errno != EINPROGRESS) {
            close(sockfd);
            return false;
        }
#endif
    }
    
    // 使用select等待连接完成，超时则失败
    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(sockfd, &write_fds);
    
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    
    result = select(sockfd + 1, nullptr, &write_fds, nullptr, &tv);
    if (result <= 0) {
#ifdef _WIN32
        closesocket(sockfd);
#else
        close(sockfd);
#endif
        return false;
    }
    
    // 连接成功后恢复为阻塞模式
#ifdef _WIN32
    mode = 0;
    ioctlsocket(sockfd, FIONBIO, &mode);
#else
    flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags & ~O_NONBLOCK);
#endif
    
    connected = true;
    
    // 如果设置了密码，进行AUTH认证
    if (!password.empty()) {
        // 构造RESP协议的AUTH命令
        std::string authCmd = "*2\r\n$4\r\nAUTH\r\n$" + std::to_string(password.length()) + "\r\n" + password + "\r\n";
        std::string response = sendCommand(authCmd);
        if (response.find("+OK") != 0) {
            disconnect();
            return false;
        }
    }
    
    // 如果指定了数据库，进行SELECT切换
    if (database != 0) {
        std::string dbStr = std::to_string(database);
        std::string selectCmd = "*2\r\n$6\r\nSELECT\r\n$" + std::to_string(dbStr.length()) + "\r\n" + dbStr + "\r\n";
        std::string response = sendCommand(selectCmd);
        if (response.find("+OK") != 0) {
            disconnect();
            return false;
        }
    }
    
    return true;
}

// 断开Redis连接
void RedisManager::disconnect() {
    std::lock_guard<std::mutex> lock(redisMutex);
    disconnectSocket();
}

// 关闭Socket
void RedisManager::disconnectSocket() {
    if (sockfd >= 0) {
#ifdef _WIN32
        closesocket(sockfd);
#else
        close(sockfd);
#endif
        sockfd = -1;
    }
    connected = false;
}

// 判断是否已连接
bool RedisManager::isConnected() const {
    return connected && sockfd >= 0;
}

// PING命令，检测Redis连接是否存活
bool RedisManager::ping() {
    if (!isConnected()) {
        return false;
    }
    
    // 构造RESP协议的PING命令
    std::string response = sendCommand("*1\r\n$4\r\nPING\r\n");
    return response.find("+PONG") == 0;
}

// 发送命令到Redis服务器
std::string RedisManager::sendCommand(const std::string& command) {
    if (!isConnected()) {
        return "";
    }
    
    // 发送命令字符串
    int sent = send(sockfd, command.c_str(), command.length(), 0);
    if (sent != static_cast<int>(command.length())) {
        return "";
    }
    
    // 读取响应
    return readResponse();
}

// 读取Redis服务器响应
std::string RedisManager::readResponse() {
    std::string response;
    char buffer[4096];
    
    while (true) {
        // 接收数据
        int received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            break;
        }
        
        buffer[received] = '\0';
        response += buffer;
        
        // 检查是否收到完整响应（以\r\n结尾）
        if (response.find("\r\n") != std::string::npos) {
            break;
        }
    }
    
    return response;
}

// 对字符串进行转义（主要处理换行符）
std::string RedisManager::escapeString(const std::string& str) {
    // Redis协议一般无需转义，特殊字符如换行符替换为空格
    std::string escaped;
    for (char c : str) {
        if (c == '\r' || c == '\n') {
            escaped += ' ';
        } else {
            escaped += c;
        }
    }
    return escaped;
}

// ================== 基本操作实现 ==================
// 设置键值，支持可选TTL
Result<bool> RedisManager::set(const std::string& key, const std::string& value, int ttl) {
    if (!isConnected()) {
        return Result<bool>::Error("Redis未连接");
    }
    
    std::string command;
    if (ttl > 0) {
        // 构造SETEX命令，带过期时间
        command = "*4\r\n$5\r\nSETEX\r\n$" + std::to_string(key.length()) + "\r\n" + key + 
                  "\r\n$" + std::to_string(std::to_string(ttl).length()) + "\r\n" + std::to_string(ttl) + 
                  "\r\n$" + std::to_string(value.length()) + "\r\n" + value + "\r\n";
    } else {
        // 构造SET命令
        command = "*3\r\n$3\r\nSET\r\n$" + std::to_string(key.length()) + "\r\n" + key + 
                  "\r\n$" + std::to_string(value.length()) + "\r\n" + value + "\r\n";
    }
    
    std::string response = sendCommand(command);
    return response.find("+OK") == 0 ? 
           Result<bool>::Success(true) : 
           Result<bool>::Error("SET操作失败: " + response);
}

// 获取键值
Result<std::string> RedisManager::get(const std::string& key) {
    if (!isConnected()) {
        return Result<std::string>::Error("Redis未连接");
    }
    
    // 构造GET命令
    std::string command = "*2\r\n$3\r\nGET\r\n$" + std::to_string(key.length()) + "\r\n" + key + "\r\n";
    std::string response = sendCommand(command);
    
    if (response.find("$-1") == 0) {
        return Result<std::string>::Error("键不存在");
    }
    
    // 解析RESP协议的Bulk String响应
    if (response.find("$") == 0) {
        size_t pos = response.find("\r\n");
        if (pos != std::string::npos) {
            std::string lengthStr = response.substr(1, pos - 1);
            int length = std::stoi(lengthStr);
            if (response.length() >= pos + 2 + length) {
                std::string value = response.substr(pos + 2, length);
                return Result<std::string>::Success(value, "获取成功");
            }
        }
    }
    
    return Result<std::string>::Error("GET操作失败: " + response);
}

// 删除键
Result<bool> RedisManager::del(const std::string& key) {
    if (!isConnected()) {
        return Result<bool>::Error("Redis未连接");
    }
    
    // 构造DEL命令
    std::string command = "*2\r\n$3\r\nDEL\r\n$" + std::to_string(key.length()) + "\r\n" + key + "\r\n";
    std::string response = sendCommand(command);
    
    if (response.find(":1") == 0) {
        return Result<bool>::Success(true);
    } else if (response.find(":0") == 0) {
        return Result<bool>::Success(false);
    }
    
    return Result<bool>::Error("DEL操作失败: " + response);
}

// 判断键是否存在
Result<bool> RedisManager::exists(const std::string& key) {
    if (!isConnected()) {
        return Result<bool>::Error("Redis未连接");
    }
    
    // 构造EXISTS命令
    std::string command = "*2\r\n$6\r\nEXISTS\r\n$" + std::to_string(key.length()) + "\r\n" + key + "\r\n";
    std::string response = sendCommand(command);
    
    if (response.find(":1") == 0) {
        return Result<bool>::Success(true);
    } else if (response.find(":0") == 0) {
        return Result<bool>::Success(false);
    }
    
    return Result<bool>::Error("EXISTS操作失败: " + response);
}

// 获取键的剩余TTL
Result<int> RedisManager::ttl(const std::string& key) {
    if (!isConnected()) {
        return Result<int>::Error("Redis未连接");
    }
    
    // 构造TTL命令
    std::string command = "*2\r\n$3\r\nTTL\r\n$" + std::to_string(key.length()) + "\r\n" + key + "\r\n";
    std::string response = sendCommand(command);
    
    if (response.find(":") == 0) {
        size_t pos = response.find("\r\n");
        if (pos != std::string::npos) {
            std::string ttlStr = response.substr(1, pos - 1);
            int ttl = std::stoi(ttlStr);
            return Result<int>::Success(ttl);
        }
    }
    
    return Result<int>::Error("TTL操作失败: " + response);
}

// 设置键的过期时间
Result<bool> RedisManager::expire(const std::string& key, int seconds) {
    if (!isConnected()) {
        return Result<bool>::Error("Redis未连接");
    }
    
    // 构造EXPIRE命令
    std::string secondsStr = std::to_string(seconds);
    std::string command = "*3\r\n$7\r\nEXPIRE\r\n$" + std::to_string(key.length()) + "\r\n" + key + 
                          "\r\n$" + std::to_string(secondsStr.length()) + "\r\n" + secondsStr + "\r\n";
    std::string response = sendCommand(command);
    
    if (response.find(":1") == 0) {
        return Result<bool>::Success(true);
    } else if (response.find(":0") == 0) {
        return Result<bool>::Success(false);
    }
    
    return Result<bool>::Error("EXPIRE操作失败: " + response);
}

// ================== 工具方法实现 ==================
// 生成缓存Key，格式为prefix:identifier
std::string RedisManager::generateCacheKey(const std::string& prefix, const std::string& identifier) {
    return prefix + ":" + identifier;
}

// ================== 会话相关方法 ==================
// 生成会话Key
std::string RedisManager::generateSessionKey(const std::string& sessionToken) {
    return generateCacheKey("session", sessionToken);
}

// 存储会话信息到Redis，序列化为JSON字符串
Result<bool> RedisManager::storeSession(const std::string& sessionToken, const Session& session, int ttl) {
    // 将Session序列化为JSON字符串
    json sessionJson;
    sessionJson["userId"] = session.userId;
    sessionJson["username"] = session.username;
    sessionJson["createdAt"] = Utils::formatTimestamp(session.createdAt);
    sessionJson["expiresAt"] = Utils::formatTimestamp(session.expiresAt);
    
    std::string sessionData = sessionJson.dump();
    std::string key = generateSessionKey(sessionToken);
    
    return set(key, sessionData, ttl);
}

// 获取会话信息，并反序列化为Session对象
Result<Session> RedisManager::getSession(const std::string& sessionToken) {
    std::string key = generateSessionKey(sessionToken);
    auto result = get(key);
    
    if (!result.success) {
        return Result<Session>::Error(result.message);
    }
    
    try {
        // 解析JSON字符串为Session对象
        json sessionJson = json::parse(result.data.value());
        Session session;
        session.userId = sessionJson["userId"];
        session.username = sessionJson["username"];
        session.createdAt = Utils::parseTimestamp(sessionJson["createdAt"]);
        session.expiresAt = Utils::parseTimestamp(sessionJson["expiresAt"]);
        
        return Result<Session>::Success(session);
    } catch (const std::exception& e) {
        return Result<Session>::Error("会话数据解析失败: " + std::string(e.what()));
    }
}

// 删除会话
Result<bool> RedisManager::deleteSession(const std::string& sessionToken) {
    std::string key = generateSessionKey(sessionToken);
    return del(key);
}

// 延长会话有效期
Result<bool> RedisManager::extendSession(const std::string& sessionToken, int ttl) {
    std::string key = generateSessionKey(sessionToken);
    return expire(key, ttl);
}

// 清空当前数据库
Result<bool> RedisManager::flushdb() {
    if (!isConnected()) {
        return Result<bool>::Error("Redis未连接");
    }
    
    // 构造FLUSHDB命令
    std::string command = "*1\r\n$7\r\nFLUSHDB\r\n";
    std::string response = sendCommand(command);
    
    if (response.find("+OK") == 0) {
        return Result<bool>::Success(true, "数据库已清空");
    }
    
    return Result<bool>::Error("FLUSHDB操作失败: " + response);
}
