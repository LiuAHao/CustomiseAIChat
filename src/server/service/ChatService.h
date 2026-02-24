#pragma once
#include <string>

class ChatService {
public:
    static ChatService& instance();

    // 创建人设
    std::string createPersona(int userId, const std::string& name, const std::string& description);
    // 删除人设
    std::string deletePersona(int userId, int personaId);
    // 获取人设列表
    std::string listPersonas(int userId);
    // 获取人设详情
    std::string getPersona(int userId, int personaId);
    // 更新人设
    std::string updatePersona(int userId, int personaId, const std::string& name, const std::string& description);
    // 获取聊天历史
    std::string getHistory(int userId, int personaId, int limit);
    // 清空聊天记录
    std::string clearHistory(int userId, int personaId);

private:
    ChatService() = default;
    std::string makeResponse(int code, const std::string& message);
};
