#pragma once
#include <string>

struct Persona {
    int id = 0;
    int userId = 0;
    std::string name;
    std::string description;   // 人设的展示描述
    std::string systemPrompt;  // 发送给AI的系统提示词
    std::string avatar;        // 人设头像标识（可选）
    std::string createdAt;
    std::string updatedAt;
};
