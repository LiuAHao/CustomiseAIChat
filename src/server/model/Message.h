#pragma once
#include <string>

struct ChatMessage {
    int id = 0;
    int personaId = 0;
    int userId = 0;
    std::string sender;
    std::string content;
    std::string timestamp;
};
