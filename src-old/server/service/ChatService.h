#pragma once
#include <string>

class ChatService {
public:
    static ChatService& instance();

    // ===== 人设管理 =====
    std::string createPersona(int userId, const std::string& name,
                              const std::string& description, const std::string& systemPrompt,
                              const std::string& avatar = "");
    std::string deletePersona(int userId, int personaId);
    std::string listPersonas(int userId);
    std::string getPersona(int userId, int personaId);
    std::string updatePersona(int userId, int personaId, const std::string& name,
                              const std::string& description, const std::string& systemPrompt,
                              const std::string& avatar = "");

    // ===== 会话管理 =====
    std::string createConversation(int userId, int personaId, const std::string& title);
    std::string deleteConversation(int userId, int conversationId);
    std::string listConversations(int userId, int limit = 50);
    std::string listConversationsByPersona(int userId, int personaId);
    std::string getConversation(int userId, int conversationId);
    std::string updateConversationTitle(int userId, int conversationId, const std::string& title);

    // ===== 聊天记录 =====
    std::string getHistory(int userId, int conversationId, int limit);
    std::string clearHistory(int userId, int conversationId);

private:
    ChatService() = default;
    std::string makeResponse(int code, const std::string& message);
};
