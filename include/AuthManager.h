#pragma once

#include "Common.h"
#include "DatabaseManager.h"
#include "RedisManager.h"
#include <mutex>

/**
 * 认证管理器类
 * 负责用户注册、登录、会话管理、密码管理等认证相关功能
 */
class AuthManager {
private:
    DatabaseManager* dbManager;                                    // 数据库管理器指针
    RedisManager* redisManager;                                    // Redis管理器指针
    std::map<std::string, Session> activeSessions;                // 活跃会话映射表 (会话令牌 -> 会话信息) - 本地缓存
    std::map<std::string, int> loginAttempts;                     // 登录尝试次数映射表 (IP/用户名 -> 尝试次数)
    mutable std::mutex sessionMutex;                              // 会话数据互斥锁
    mutable std::mutex attemptMutex;                              // 登录尝试数据互斥锁

    std::string currentSessionToken;                              // 当前会话令牌
    int currentUserId;                                            // 当前用户ID
    bool isLoggedIn;                                              // 当前登录状态

    /**
     * 生成新的会话令牌
     * @return 生成的会话令牌字符串
     */
    std::string generateSessionToken();
    
    /**
     * 检查会话是否已过期
     * @param session 要检查的会话对象
     * @return 如果会话已过期返回true，否则返回false
     */
    bool isSessionExpired(const Session& session) const;
    

    
    /**
     * 检查账户是否被锁定
     * @param username 用户名
     * @return 如果账户被锁定返回true，否则返回false
     */
    bool isAccountLocked(const std::string& username) const;
    
    /**
     * 记录失败的登录尝试
     * @param username 登录失败的用户名
     */
    void recordFailedLogin(const std::string& username);
    
    /**
     * 重置用户的登录尝试次数
     * @param username 用户名
     */
    void resetLoginAttempts(const std::string& username);

public:

    /**
 * 清理过期的会话
 * 自动移除所有已过期的会话记录
 */
    void cleanupExpiredSessions();
    /**
     * 构造函数
     * @param db 数据库管理器指针
     * @param redis Redis管理器指针（可选）
     */
    explicit AuthManager(DatabaseManager* db, RedisManager* redis = nullptr);
    
    /**
     * 析构函数
     */
    ~AuthManager();

    // ==================== 用户注册和认证 ====================
    
    /**
     * 用户注册
     * 创建新用户账户，包括用户名、密码和邮箱验证
     * @param username 用户名（必须唯一）
     * @param password 密码（将自动加密存储）
     * @param email 邮箱地址（必须唯一）
     * @return 注册结果，成功时返回用户信息，失败时返回错误信息
     */
    Result<User> registerUser(const std::string& username, const std::string& password,
                              const std::string& email);
    
    /**
     * 用户登录
     * 验证用户名和密码，创建新的会话
     * @param username 用户名
     * @param password 密码
     * @return 登录结果，成功时返回会话令牌，失败时返回错误信息
     */
    Result<std::string> login(const std::string& username, const std::string& password);
    
    /**
     * 用户登出（当前会话）
     * 清除当前用户的会话信息
     * @return 登出结果，成功返回true，失败返回false
     */
    Result<bool> logout();
    
    /**
     * 用户登出（指定会话）
     * 根据会话令牌清除指定的会话
     * @param sessionToken 要清除的会话令牌
     * @return 登出结果，成功返回true，失败返回false
     */
    Result<bool> logout(const std::string& sessionToken);

    // ==================== 会话管理 ====================
    
    /**
     * 获取当前登录用户信息
     * @return 当前用户信息，如果未登录则返回错误
     */
    Result<User> getCurrentUser() const;
    
    /**
     * 根据会话令牌获取用户信息
     * @param sessionToken 会话令牌
     * @return 用户信息，如果会话无效则返回错误
     */
    Result<User> getUserFromSession(const std::string& sessionToken) const;
    
    /**
     * 验证会话是否有效
     * @param sessionToken 会话令牌
     * @return 如果会话有效且未过期返回true，否则返回false
     */
    bool isValidSession(const std::string& sessionToken) const;
    
    /**
     * 检查当前是否有用户登录
     * @return 如果当前有用户登录返回true，否则返回false
     */
    bool isCurrentlyLoggedIn() const;
    
    /**
     * 获取当前会话令牌
     * @return 当前会话令牌，如果未登录则返回空字符串
     */
    std::string getCurrentSessionToken() const;

    // ==================== 密码管理 ====================
    
    /**
     * 修改当前用户密码
     * @param oldPassword 旧密码
     * @param newPassword 新密码
     * @return 修改结果，成功返回true，失败返回false
     */
    Result<bool> changePassword(const std::string& oldPassword, const std::string& newPassword);
    
    /**
     * 管理员修改指定用户密码
     * @param userId 用户ID
     * @param newPassword 新密码
     * @return 修改结果，成功返回true，失败返回false
     */
    Result<bool> changePassword(int userId, const std::string& newPassword);
    
    /**
     * 密码哈希加密
     * 使用安全的哈希算法对密码进行加密
     * @param password 原始密码
     * @return 加密后的密码哈希值
     */
    static std::string hashPassword(const std::string& password);
    
    /**
     * 验证密码
     * 验证输入的密码是否与存储的哈希值匹配
     * @param password 要验证的密码
     * @param hash 存储的密码哈希值
     * @return 如果密码匹配返回true，否则返回false
     */
    static bool verifyPassword(const std::string& password, const std::string& hash);

    // ==================== 账户管理 ====================
    
    /**
     * 禁用用户账户
     * @param userId 要禁用的用户ID
     * @return 操作结果，成功返回true，失败返回false
     */
    Result<bool> deactivateUser(int userId);
    
    /**
     * 激活用户账户
     * @param userId 要激活的用户ID
     * @return 操作结果，成功返回true，失败返回false
     */
    Result<bool> activateUser(int userId);
    
    /**
     * 删除用户账户
     * 注意：此操作不可逆，会同时清除用户的所有会话
     * @param userId 要删除的用户ID
     * @return 操作结果，成功返回true，失败返回false
     */
    Result<bool> deleteUser(int userId);

    // ==================== 会话工具 ====================
    
    /**
     * 列出所有活跃会话
     * 在控制台输出当前所有活跃会话的详细信息
     */
    void listActiveSessions() const;
    
    /**
     * 获取活跃会话数量
     * @return 当前活跃会话的总数
     */
    int getActiveSessionCount() const;
    
    /**
     * 清除所有会话
     * 强制所有用户重新登录
     * @return 操作结果，成功返回true，失败返回false
     */
    Result<bool> invalidateAllSessions();
    
    /**
     * 清除指定用户的所有会话
     * @param userId 用户ID
     * @return 操作结果，成功返回true，失败返回false
     */
    Result<bool> invalidateUserSessions(int userId);

    // ==================== 安全工具 ====================
    
    /**
     * 验证密码强度
     * 检查密码是否符合安全要求
     * @param password 要验证的密码
     * @return 如果密码符合强度要求返回true，否则返回false
     */
    bool validatePasswordStrength(const std::string& password) const;
    
    /**
     * 获取密码要求列表
     * @return 密码必须满足的要求列表
     */
    std::vector<std::string> getPasswordRequirements() const;
    
    /**
     * 清除所有登录尝试记录
     * 重置所有用户的登录失败计数
     */
    void clearLoginAttempts();

    // ==================== 管理员功能 ====================
    
    /**
     * 获取所有用户列表
     * @return 所有用户的列表，失败时返回错误信息
     */
    Result<std::vector<User>> getAllUsers() const;
    
    /**
     * 检查用户是否为管理员
     * 为未来的基于角色的访问控制预留
     * @param userId 用户ID
     * @return 如果用户是管理员返回true，否则返回false
     */
    bool isUserAdmin(int userId) const;
};