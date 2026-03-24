#include "UserService.h"
#include "../db/Database.h"
#include <jsoncpp/json/json.h>
#include <random>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>
#include <iostream>

UserService& UserService::instance() {
    static UserService svc;
    return svc;
}

std::string UserService::generateSalt() {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);

    std::string salt(16, '\0');
    for (auto& c : salt) {
        c = charset[dis(gen)];
    }
    return salt;
}

std::string UserService::generateToken() {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);

    std::string token(48, '\0');
    for (auto& c : token) {
        c = charset[dis(gen)];
    }
    return token;
}

std::string UserService::hashPassword(const std::string& password, const std::string& salt) {
    std::string input = salt + password + salt;
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, input.c_str(), input.size());
    EVP_DigestFinal_ex(ctx, hash, &hashLen);
    EVP_MD_CTX_free(ctx);

    std::ostringstream oss;
    for (unsigned int i = 0; i < hashLen; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return oss.str();
}

std::string UserService::makeResponse(int code, const std::string& message) {
    Json::Value resp;
    resp["code"] = code;
    resp["message"] = message;
    Json::FastWriter writer;
    return writer.write(resp);
}

std::string UserService::registerUser(const std::string& username, const std::string& password,
                                       const std::string& nickname) {
    // 参数校验
    if (username.empty() || username.size() < 3 || username.size() > 32) {
        return makeResponse(-1, "用户名长度需在3-32个字符之间");
    }
    if (password.empty() || password.size() < 6) {
        return makeResponse(-1, "密码长度至少6个字符");
    }
    if (nickname.empty()) {
        return makeResponse(-1, "昵称不能为空");
    }

    // 检查用户名是否已存在
    User existing;
    if (Database::instance().getUserByUsername(username, existing)) {
        return makeResponse(-1, "用户名已被注册");
    }

    // 创建用户
    std::string salt = generateSalt();
    std::string hash = hashPassword(password, salt);
    int userId = Database::instance().createUser(username, hash, salt, nickname);

    if (userId < 0) {
        return makeResponse(-1, "注册失败，请稍后重试");
    }

    Json::Value resp;
    resp["code"] = 0;
    resp["message"] = "注册成功";
    resp["userId"] = userId;
    Json::FastWriter writer;

    std::cout << "[UserService] 新用户注册: " << username << " (ID:" << userId << ")" << std::endl;
    return writer.write(resp);
}

std::string UserService::loginUser(const std::string& username, const std::string& password) {
    if (username.empty() || password.empty()) {
        return makeResponse(-1, "用户名和密码不能为空");
    }

    User user;
    if (!Database::instance().getUserByUsername(username, user)) {
        return makeResponse(-1, "用户名或密码错误");
    }

    std::string hash = hashPassword(password, user.salt);
    if (hash != user.passwordHash) {
        return makeResponse(-1, "用户名或密码错误");
    }

    // 生成会话token
    std::string token = generateToken();
    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        // 清除该用户的旧会话
        for (auto it = sessions_.begin(); it != sessions_.end(); ) {
            if (it->second == user.id) {
                it = sessions_.erase(it);
            } else {
                ++it;
            }
        }
        sessions_[token] = user.id;
    }

    Json::Value resp;
    resp["code"] = 0;
    resp["message"] = "登录成功";
    resp["token"] = token;
    resp["userId"] = user.id;
    resp["nickname"] = user.nickname;
    Json::FastWriter writer;

    std::cout << "[UserService] 用户登录: " << username << " (ID:" << user.id << ")" << std::endl;
    return writer.write(resp);
}

std::string UserService::logoutUser(const std::string& token) {
    std::lock_guard<std::mutex> lock(sessionMutex_);
    auto it = sessions_.find(token);
    if (it != sessions_.end()) {
        std::cout << "[UserService] 用户登出 (ID:" << it->second << ")" << std::endl;
        sessions_.erase(it);
    }
    return makeResponse(0, "已登出");
}

std::string UserService::updatePassword(const std::string& token, const std::string& oldPwd,
                                         const std::string& newPwd) {
    int userId = getUserIdByToken(token);
    if (userId <= 0) {
        return makeResponse(-2, "未登录或会话已过期");
    }

    if (newPwd.empty() || newPwd.size() < 6) {
        return makeResponse(-1, "新密码长度至少6个字符");
    }

    User user;
    if (!Database::instance().getUserById(userId, user)) {
        return makeResponse(-1, "用户不存在");
    }

    // 验证旧密码
    std::string oldHash = hashPassword(oldPwd, user.salt);
    if (oldHash != user.passwordHash) {
        return makeResponse(-1, "旧密码不正确");
    }

    // 更新密码
    std::string newSalt = generateSalt();
    std::string newHash = hashPassword(newPwd, newSalt);
    if (!Database::instance().updatePassword(userId, newHash, newSalt)) {
        return makeResponse(-1, "修改密码失败");
    }

    return makeResponse(0, "密码修改成功");
}

std::string UserService::updateNickname(const std::string& token, const std::string& nickname) {
    int userId = getUserIdByToken(token);
    if (userId <= 0) {
        return makeResponse(-2, "未登录或会话已过期");
    }

    if (nickname.empty()) {
        return makeResponse(-1, "昵称不能为空");
    }

    if (!Database::instance().updateNickname(userId, nickname)) {
        return makeResponse(-1, "修改昵称失败");
    }

    return makeResponse(0, "昵称修改成功");
}

std::string UserService::getUserInfo(const std::string& token) {
    int userId = getUserIdByToken(token);
    if (userId <= 0) {
        return makeResponse(-2, "未登录或会话已过期");
    }

    User user;
    if (!Database::instance().getUserById(userId, user)) {
        return makeResponse(-1, "用户不存在");
    }

    Json::Value resp;
    resp["code"] = 0;
    resp["userId"] = user.id;
    resp["username"] = user.username;
    resp["nickname"] = user.nickname;
    resp["createdAt"] = user.createdAt;
    Json::FastWriter writer;
    return writer.write(resp);
}

std::string UserService::deleteAccount(const std::string& token, const std::string& password) {
    int userId = getUserIdByToken(token);
    if (userId <= 0) {
        return makeResponse(-2, "未登录或会话已过期");
    }

    User user;
    if (!Database::instance().getUserById(userId, user)) {
        return makeResponse(-1, "用户不存在");
    }

    // 验证密码
    std::string hash = hashPassword(password, user.salt);
    if (hash != user.passwordHash) {
        return makeResponse(-1, "密码不正确");
    }

    if (!Database::instance().deleteUser(userId)) {
        return makeResponse(-1, "删除账号失败");
    }

    // 清除会话
    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        for (auto it = sessions_.begin(); it != sessions_.end(); ) {
            if (it->second == userId) {
                it = sessions_.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::cout << "[UserService] 用户注销账号: " << user.username << " (ID:" << userId << ")" << std::endl;
    return makeResponse(0, "账号已注销");
}

int UserService::getUserIdByToken(const std::string& token) {
    if (token.empty()) return -1;
    std::lock_guard<std::mutex> lock(sessionMutex_);
    auto it = sessions_.find(token);
    if (it != sessions_.end()) {
        return it->second;
    }
    return -1;
}

bool UserService::isValidToken(const std::string& token) {
    return getUserIdByToken(token) > 0;
}
