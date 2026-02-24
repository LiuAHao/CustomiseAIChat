#pragma once
#include <string>
#include <cstdint>

struct ServerConfig {
    // 网络设置
    std::string ip = "0.0.0.0";
    uint16_t port = 5085;
    int ioThreads = 3;
    int workThreads = 5;

    // 数据库设置
    std::string dbPath = "./data/chat.db";

    // AI API 设置
    std::string aiApiKey;
    std::string aiApiUrl = "https://api.deepseek.com/v1/chat/completions";
    std::string aiModel = "deepseek-chat";

    // 聊天设置
    int maxHistoryContext = 10;  // AI上下文携带的最大历史消息数

    static ServerConfig& instance();
    bool loadFromFile(const std::string& path);
    void print() const;
};
