#pragma once

#include "Common.h"

/**
 * Redis管理器类
 * 使用原始TCP socket实现的简单Redis客户端
 * 主要用于会话存储和管理
 * 生产环境建议使用 cpp-redis 或 hiredis 库
 */
class RedisManager {
private:
    std::string host;                    // Redis服务器地址
    int port;                            // Redis服务器端口
    std::string password;                // Redis认证密码
    int database;                        // Redis数据库编号
    int timeout;                         // 连接超时时间（毫秒）

    int sockfd;                          // Socket文件描述符
    bool connected;                      // 连接状态标志
    std::mutex redisMutex;               // Redis操作互斥锁

    /**
     * 建立Socket连接
     * @return 连接成功返回true，失败返回false
     */
    bool connectSocket();
    
    /**
     * 断开Socket连接
     */
    void disconnectSocket();
    
    /**
     * 发送Redis命令
     * @param command 要发送的命令字符串
     * @return Redis服务器的响应
     */
    std::string sendCommand(const std::string& command);
    
    /**
     * 读取Redis响应
     * @return Redis服务器的响应字符串
     */
    std::string readResponse();
    
    /**
     * 字符串转义处理
     * @param str 需要转义的字符串
     * @return 转义后的字符串
     */
    std::string escapeString(const std::string& str);

public:
    /**
     * 构造函数
     * 初始化Redis连接参数，默认连接到本地6379端口
     */
    RedisManager();
    
    /**
     * 析构函数
     * 自动断开Redis连接并清理资源
     */
    ~RedisManager();

    /**
     * 连接到Redis服务器
     * @param host Redis服务器地址
     * @param port Redis服务器端口
     * @param password Redis认证密码（可选）
     * @param database 要使用的数据库编号（默认0）
     * @param timeout 连接超时时间（毫秒，默认5000）
     * @return 连接成功返回true，失败返回false
     */
    bool connect(const std::string& host, int port, const std::string& password = "",
                 int database = 0, int timeout = 5000);
    
    /**
     * 断开Redis连接
     * 关闭socket连接并重置连接状态
     */
    void disconnect();
    
    /**
     * 检查Redis连接状态
     * @return 如果已连接返回true，否则返回false
     */
    bool isConnected() const;
    
    /**
     * 测试Redis连接
     * 发送PING命令测试服务器响应
     * @return 服务器响应PONG返回true，否则返回false
     */
    bool ping();

    // ==================== 基本操作 ====================
    
    /**
     * 设置键值对
     * @param key 键名
     * @param value 值
     * @param ttl 生存时间（秒，0表示永不过期）
     * @return 操作结果，成功返回true，失败返回false
     */
    Result<bool> set(const std::string& key, const std::string& value, int ttl = 0);
    
    /**
     * 获取键值
     * @param key 键名
     * @return 操作结果，成功返回值，失败返回错误信息
     */
    Result<std::string> get(const std::string& key);
    
    /**
     * 删除键
     * @param key 要删除的键名
     * @return 操作结果，成功返回true，失败返回false
     */
    Result<bool> del(const std::string& key);
    
    /**
     * 检查键是否存在
     * @param key 要检查的键名
     * @return 操作结果，键存在返回true，不存在返回false
     */
    Result<bool> exists(const std::string& key);
    
    /**
     * 获取键的剩余生存时间
     * @param key 键名
     * @return 操作结果，成功返回剩余秒数，-1表示永不过期，-2表示键不存在
     */
    Result<int> ttl(const std::string& key);
    
    /**
     * 设置键的过期时间
     * @param key 键名
     * @param seconds 过期时间（秒）
     * @return 操作结果，成功返回true，失败返回false
     */
    Result<bool> expire(const std::string& key, int seconds);

    // ==================== 统计和监控 ====================
    
    /**
     * 清空当前数据库
     * 删除当前数据库中的所有键
     * @return 操作结果，成功返回true，失败返回false
     */
    Result<bool> flushdb();

    // ==================== 工具函数 ====================
    
    /**
     * 生成缓存键
     * 根据前缀和标识符生成标准的缓存键格式
     * @param prefix 键前缀
     * @param identifier 标识符
     * @return 生成的缓存键字符串
     */
    std::string generateCacheKey(const std::string& prefix, const std::string& identifier);
    
    // ==================== 会话管理 ====================
    
    /**
     * 生成会话键
     * 根据会话令牌生成Redis中的会话键
     * @param sessionToken 会话令牌
     * @return 生成的会话键字符串
     */
    std::string generateSessionKey(const std::string& sessionToken);
    
    /**
     * 存储会话信息
     * 将会话对象序列化为JSON并存储到Redis
     * @param sessionToken 会话令牌
     * @param session 会话对象
     * @param ttl 会话生存时间（秒，默认24小时）
     * @return 操作结果，成功返回true，失败返回false
     */
    Result<bool> storeSession(const std::string& sessionToken, const Session& session, int ttl = 86400);
    
    /**
     * 获取会话信息
     * 从Redis中获取会话数据并反序列化为Session对象
     * @param sessionToken 会话令牌
     * @return 操作结果，成功返回Session对象，失败返回错误信息
     */
    Result<Session> getSession(const std::string& sessionToken);
    
    /**
     * 删除会话
     * 从Redis中删除指定的会话
     * @param sessionToken 会话令牌
     * @return 操作结果，成功返回true，失败返回false
     */
    Result<bool> deleteSession(const std::string& sessionToken);
    
    /**
     * 延长会话时间
     * 更新会话的过期时间
     * @param sessionToken 会话令牌
     * @param ttl 新的生存时间（秒）
     * @return 操作结果，成功返回true，失败返回false
     */
    Result<bool> extendSession(const std::string& sessionToken, int ttl);
};