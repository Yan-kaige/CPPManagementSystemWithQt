#include "AuthManager.h"
#include <regex>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>

AuthManager::AuthManager(DatabaseManager* db, RedisManager* redis)
        : dbManager(db), redisManager(redis), isLoggedIn(false), currentUserId(-1) {
    // 其他初始化逻辑可写可不写
}

AuthManager::~AuthManager() {
    // 如果没有资源释放，可以为空
}

std::string AuthManager::hashPassword(const std::string& password) {
    return Utils::sha256Hash(password);
}

bool AuthManager::verifyPassword(const std::string& password, const std::string& hash) {
    std::string computedHash = hashPassword(password);
    return computedHash == hash;
}

std::string AuthManager::generateSessionToken() {
    return Utils::generateRandomString(32);
}

bool AuthManager::isSessionExpired(const Session& session) const {
    auto now = std::chrono::system_clock::now();
    return now > session.expiresAt;
}

void AuthManager::cleanupExpiredSessions() {
    std::lock_guard<std::mutex> lock(sessionMutex);
    auto it = activeSessions.begin();
    while (it != activeSessions.end()) {
        if (isSessionExpired(it->second)) {
            it = activeSessions.erase(it);
        } else {
            ++it;
        }
    }
}

bool AuthManager::isAccountLocked(const std::string& username) const {
    std::lock_guard<std::mutex> lock(attemptMutex);
    auto it = loginAttempts.find(username);
    if (it != loginAttempts.end()) {
        return it->second >= 5; // 5次失败后锁定
    }
    return false;
}

void AuthManager::recordFailedLogin(const std::string& username) {
    std::lock_guard<std::mutex> lock(attemptMutex);
    loginAttempts[username]++;
}

void AuthManager::resetLoginAttempts(const std::string& username) {
    std::lock_guard<std::mutex> lock(attemptMutex);
    loginAttempts.erase(username);
}

Result<User> AuthManager::registerUser(const std::string& username, const std::string& password, const std::string& email) {
    // 参数校验
    if (username.empty() || password.empty() || email.empty()) {
        return Result<User>::Error("用户名、密码、邮箱均不能为空");
    }
    // 邮箱格式校验
    if (!Utils::isValidEmail(email)) {
        return Result<User>::Error("邮箱格式不正确");
    }
    // 检查用户名是否已存在
    auto userRes = dbManager->getUserByUsername(username);
    if (userRes.success) {
        return Result<User>::Error("用户名已存在");
    }
    // 检查邮箱是否已被注册
    // 这里可以扩展为 getUserByEmail，如果有实现
    // 密码hash
    std::string hash = hashPassword(password);
    // 写入数据库
    auto createRes = dbManager->createUser(username, hash, email);
    if (!createRes.success) {
        return Result<User>::Error("数据库写入失败: " + createRes.message);
    }
    return Result<User>::Success(createRes.data.value(), "注册成功");
}

Result<std::string> AuthManager::login(const std::string& username, const std::string& password) {
    // 检查账户是否被锁定
    if (isAccountLocked(username)) {
        return Result<std::string>::Error("账户已被锁定，请稍后再试");
    }
    
    // 获取用户信息
    auto userRes = dbManager->getUserByUsername(username);
    if (!userRes.success) {
        recordFailedLogin(username);
        return Result<std::string>::Error("用户名或密码错误");
    }
    
    User user = userRes.data.value();
    
    // 检查用户是否激活
    if (!user.is_active) {
        return Result<std::string>::Error("账户已被禁用");
    }
    
    // 验证密码
    if (!verifyPassword(password, user.password_hash)) {
        recordFailedLogin(username);
        return Result<std::string>::Error("用户名或密码错误");
    }
    
    // 密码正确，重置登录尝试次数
    resetLoginAttempts(username);
    
    // 生成会话令牌
    std::string sessionToken = generateSessionToken();
    
    // 创建会话
    Session session;
    session.userId = user.id;
    session.username = user.username;
    session.createdAt = std::chrono::system_clock::now();
    session.expiresAt = session.createdAt + std::chrono::hours(24); // 24小时过期
    
    // 保存会话到Redis（如果可用）
    if (redisManager && redisManager->isConnected()) {
        auto redisResult = redisManager->storeSession(sessionToken, session, 86400); // 24小时过期
        if (!redisResult.success) {
            // Redis存储失败，记录日志但继续使用本地存储
            std::cerr << "Redis会话存储失败: " << redisResult.message << std::endl;
        }
    }
    
    // 同时保存到本地缓存
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        activeSessions[sessionToken] = session;
    }
    
    // 更新当前登录状态
    currentSessionToken = sessionToken;
    currentUserId = user.id;
    isLoggedIn = true;
    
    // 更新最后登录时间
    dbManager->updateUserLastLogin(user.id);
    
    return Result<std::string>::Success(sessionToken, "登录成功");
}

Result<bool> AuthManager::logout() {
    if (!isLoggedIn) {
        return Result<bool>::Error("当前未登录");
    }
    
    return logout(currentSessionToken);
}

Result<bool> AuthManager::logout(const std::string& sessionToken) {
    // 首先尝试从Redis删除会话
    if (redisManager && redisManager->isConnected()) {
        auto redisResult = redisManager->deleteSession(sessionToken);
        if (!redisResult.success) {
            // Redis删除失败，记录日志但继续处理本地存储
            std::cerr << "Redis会话删除失败: " << redisResult.message << std::endl;
        }
    }
    
    std::lock_guard<std::mutex> lock(sessionMutex);
    
    auto it = activeSessions.find(sessionToken);
    if (it != activeSessions.end()) {
        activeSessions.erase(it);
        
        // 如果是当前会话，清除当前登录状态
        if (sessionToken == currentSessionToken) {
            currentSessionToken.clear();
            currentUserId = -1;
            isLoggedIn = false;
        }
        
        return Result<bool>::Success(true, "登出成功");
    }
    
    return Result<bool>::Error("会话不存在");
}

Result<User> AuthManager::getCurrentUser() const {
    if (!isLoggedIn) {
        return Result<User>::Error("当前未登录");
    }
    
    return dbManager->getUserById(currentUserId);
}

Result<User> AuthManager::getUserFromSession(const std::string& sessionToken) const {
    // 首先尝试从Redis获取会话
    if (redisManager && redisManager->isConnected()) {
        auto redisResult = redisManager->getSession(sessionToken);
        if (redisResult.success) {
            Session session = redisResult.data.value();
            if (isSessionExpired(session)) {
                return Result<User>::Error("会话已过期");
            }
            
            auto userResult = dbManager->getUserById(session.userId);
            if (userResult.success) {
                return userResult;
            } else {
                return Result<User>::Error("用户不存在");
            }
        }
    }
    
    // 如果Redis不可用或会话不存在，尝试本地缓存
    std::lock_guard<std::mutex> lock(sessionMutex);
    
    auto it = activeSessions.find(sessionToken);
    if (it != activeSessions.end()) {
        const Session& session = it->second;
        
        // 检查会话是否过期
        if (isSessionExpired(session)) {
            return Result<User>::Error("会话已过期");
        }
        
        return dbManager->getUserById(session.userId);
    }
    
    return Result<User>::Error("会话不存在");
}

bool AuthManager::isValidSession(const std::string& sessionToken) const {
    // 首先尝试从Redis验证会话
    if (redisManager && redisManager->isConnected()) {
        auto redisResult = redisManager->getSession(sessionToken);
        if (redisResult.success) {
            Session session = redisResult.data.value();
            return !isSessionExpired(session);
        }
    }
    
    // 如果Redis不可用，尝试本地缓存
    std::lock_guard<std::mutex> lock(sessionMutex);
    
    auto it = activeSessions.find(sessionToken);
    if (it != activeSessions.end()) {
        return !isSessionExpired(it->second);
    }
    
    return false;
}

bool AuthManager::isCurrentlyLoggedIn() const {
    return isLoggedIn;
}

std::string AuthManager::getCurrentSessionToken() const {
    return currentSessionToken;
}

Result<bool> AuthManager::changePassword(const std::string& oldPassword, const std::string& newPassword) {
    if (!isLoggedIn) {
        return Result<bool>::Error("当前未登录");
    }
    
    // 获取当前用户
    auto userRes = getCurrentUser();
    if (!userRes.success) {
        return Result<bool>::Error("获取用户信息失败");
    }
    
    User user = userRes.data.value();
    
    // 验证旧密码
    if (!verifyPassword(oldPassword, user.password_hash)) {
        return Result<bool>::Error("旧密码错误");
    }
    
    // 验证新密码强度
    if (!validatePasswordStrength(newPassword)) {
        return Result<bool>::Error("新密码不符合要求");
    }
    
    // 更新密码
    return changePassword(user.id, newPassword);
}

Result<bool> AuthManager::changePassword(int userId, const std::string& newPassword) {
    // 获取用户信息
    auto userRes = dbManager->getUserById(userId);
    if (!userRes.success) {
        return Result<bool>::Error("用户不存在");
    }
    
    User user = userRes.data.value();
    
    // 生成新密码哈希
    std::string newHash = hashPassword(newPassword);
    
    // 更新用户密码
    user.password_hash = newHash;
    auto updateRes = dbManager->updateUser(user);
    
    if (updateRes.success) {
        // 如果是当前用户，清除所有会话（强制重新登录）
        if (userId == currentUserId) {
            invalidateUserSessions(userId);
            logout();
        }
        return Result<bool>::Success(true, "密码修改成功");
    } else {
        return Result<bool>::Error("密码修改失败: " + updateRes.message);
    }
}

bool AuthManager::validatePasswordStrength(const std::string& password) const {
    // 密码长度至少6位
    if (password.length() < 6) {
        return false;
    }
    
    // 检查是否包含数字和字母
    bool hasDigit = false;
    bool hasLetter = false;
    
    for (char c : password) {
        if (std::isdigit(c)) hasDigit = true;
        if (std::isalpha(c)) hasLetter = true;
    }
    
    return hasDigit && hasLetter;
}

std::vector<std::string> AuthManager::getPasswordRequirements() const {
    return {
        "密码长度至少6位",
        "必须包含数字和字母"
    };
}

void AuthManager::clearLoginAttempts() {
    std::lock_guard<std::mutex> lock(attemptMutex);
    loginAttempts.clear();
}

void AuthManager::listActiveSessions() const {
    std::lock_guard<std::mutex> lock(sessionMutex);
    
    std::cout << "活跃会话列表:" << std::endl;
    for (const auto& pair : activeSessions) {
        const Session& session = pair.second;
        auto now = std::chrono::system_clock::now();
        auto remaining = std::chrono::duration_cast<std::chrono::hours>(session.expiresAt - now);
        
        std::cout << "令牌: " << pair.first.substr(0, 8) << "..."
                  << " 用户: " << session.username
                  << " 剩余时间: " << remaining.count() << "小时" << std::endl;
    }
}

int AuthManager::getActiveSessionCount() const {
    std::lock_guard<std::mutex> lock(sessionMutex);
    return activeSessions.size();
}

Result<bool> AuthManager::invalidateAllSessions() {
    // 清除Redis中的所有会话
    if (redisManager && redisManager->isConnected()) {
        auto redisResult = redisManager->flushdb();
        if (!redisResult.success) {
            std::cerr << "Redis清除失败: " << redisResult.message << std::endl;
        }
    }
    
    // 清除本地缓存
    std::lock_guard<std::mutex> lock(sessionMutex);
    activeSessions.clear();
    
    // 清除当前登录状态
    currentSessionToken.clear();
    currentUserId = -1;
    isLoggedIn = false;
    
    return Result<bool>::Success(true, "所有会话已清除");
}

Result<bool> AuthManager::invalidateUserSessions(int userId) {
    std::lock_guard<std::mutex> lock(sessionMutex);
    
    auto it = activeSessions.begin();
    while (it != activeSessions.end()) {
        if (it->second.userId == userId) {
            it = activeSessions.erase(it);
        } else {
            ++it;
        }
    }
    
    // 如果清除的是当前用户，清除当前登录状态
    if (userId == currentUserId) {
        currentSessionToken.clear();
        currentUserId = -1;
        isLoggedIn = false;
    }
    
    return Result<bool>::Success(true, "用户会话已清除");
}

Result<std::vector<User>> AuthManager::getAllUsers() const {
    return dbManager->getAllUsers();
}

bool AuthManager::isUserAdmin(int userId) const {
    // 简单的管理员检查，可以根据需要扩展
    // 这里假设用户ID为1的是管理员
    return userId == 1;
}

Result<bool> AuthManager::deactivateUser(int userId) {
    auto userRes = dbManager->getUserById(userId);
    if (!userRes.success) {
        return Result<bool>::Error("用户不存在");
    }
    
    User user = userRes.data.value();
    user.is_active = false;
    
    auto updateRes = dbManager->updateUser(user);
    if (updateRes.success) {
        // 如果禁用的是当前用户，清除会话
        if (userId == currentUserId) {
            invalidateUserSessions(userId);
            logout();
        }
        return Result<bool>::Success(true, "用户已禁用");
    } else {
        return Result<bool>::Error("禁用用户失败: " + updateRes.message);
    }
}

Result<bool> AuthManager::activateUser(int userId) {
    auto userRes = dbManager->getUserById(userId);
    if (!userRes.success) {
        return Result<bool>::Error("用户不存在");
    }
    
    User user = userRes.data.value();
    user.is_active = true;
    
    auto updateRes = dbManager->updateUser(user);
    if (updateRes.success) {
        return Result<bool>::Success(true, "用户已激活");
    } else {
        return Result<bool>::Error("激活用户失败: " + updateRes.message);
    }
}

Result<bool> AuthManager::deleteUser(int userId) {
    // 不能删除当前登录用户
    if (userId == currentUserId) {
        return Result<bool>::Error("不能删除当前登录用户");
    }
    
    auto deleteRes = dbManager->deleteUser(userId);
    if (deleteRes.success) {
        // 清除该用户的所有会话
        invalidateUserSessions(userId);
        return Result<bool>::Success(true, "用户已删除");
    } else {
        return Result<bool>::Error("删除用户失败: " + deleteRes.message);
    }
}
