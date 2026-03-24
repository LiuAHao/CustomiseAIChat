#pragma once
#include <string>
#include <vector>
#include "../model/Message.h"

class AIService {
public:
    static AIService& instance();

    // 发送消息给AI并获取回复（基于会话）
    std::string sendMessage(int userId, int conversationId, const std::string& message);

    // 新建会话后让人设主动打招呼开始对话
    std::string sendGreeting(int userId, int conversationId);

    // AI生成人设描述
    std::string generateDescription(int userId, const std::string& personaName);

private:
    AIService() = default;

    // 调用AI API
    std::string callAIAPI(const std::string& systemPrompt,
                          const std::vector<ChatMessage>& history,
                          const std::string& userMessage);
};
