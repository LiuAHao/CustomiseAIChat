#pragma once
#include <string>
#include <vector>
#include "../model/Message.h"

class AIService {
public:
    static AIService& instance();

    // 发送消息给AI并获取回复，同时保存聊天记录
    std::string sendMessage(int userId, int personaId, const std::string& message);

private:
    AIService() = default;

    // 调用AI API
    std::string callAIAPI(const std::string& systemPrompt,
                          const std::vector<ChatMessage>& history,
                          const std::string& userMessage);
};
