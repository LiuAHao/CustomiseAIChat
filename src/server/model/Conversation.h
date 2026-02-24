#pragma once
#include <string>

struct Conversation {
    int id = 0;
    int userId = 0;      // 所属用户
    int personaId = 0;   // 使用的人设
    std::string title;    // 会话标题（可由首条消息自动生成）
    int messageCount = 0; // 消息数量（冗余缓存，方便展示）
    std::string createdAt;
    std::string updatedAt;
};
