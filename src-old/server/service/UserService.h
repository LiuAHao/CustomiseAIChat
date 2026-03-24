#pragma once
#include <string>
#include <map>
#include <mutex>

class UserService {
public:
    static UserService& instance();

    // 用户注册 - 返回JSON响应
    std::string registerUser(const std::string& username, const std::string& password,
                             const std::string& nickname);
    // 用户登录
    std::string loginUser(const std::string& username, const std::string& password);
    // 用户登出
    std::string logoutUser(const std::string& token);
    // 修改密码
    std::string updatePassword(const std::string& token, const std::string& oldPwd,
                               const std::string& newPwd);
    // 修改昵称
    std::string updateNickname(const std::string& token, const std::string& nickname);
    // 获取用户信息
    std::string getUserInfo(const std::string& token);
    // 注销账号
    std::string deleteAccount(const std::string& token, const std::string& password);

    // Token验证
    int getUserIdByToken(const std::string& token);
    bool isValidToken(const std::string& token);

private:
    UserService() = default;

    std::string generateToken();
    std::string generateSalt();
    std::string hashPassword(const std::string& password, const std::string& salt);
    std::string makeResponse(int code, const std::string& message);

    std::map<std::string, int> sessions_;  // token -> userId
    std::mutex sessionMutex_;
};
