#pragma once
#include <string>

struct ChatMessage {
    int id = 0;
    int conversationId = 0;  // 所属会话
    std::string role;         // "user" / "assistant" / "system"
    std::string content;
    std::string timestamp;
};
