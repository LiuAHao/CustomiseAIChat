#include "ServerConfig.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>

ServerConfig& ServerConfig::instance() {
    static ServerConfig config;
    return config;
}

static std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

static std::string stripQuotes(const std::string& value) {
    if (value.size() >= 2) {
        if ((value.front() == '"' && value.back() == '"') ||
            (value.front() == '\'' && value.back() == '\'')) {
            return value.substr(1, value.size() - 2);
        }
    }
    return value;
}

static int safeToInt(const std::string& value, int fallback) {
    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

static std::string normalizeAiUrl(const std::string& rawUrl) {
    std::string url = trim(rawUrl);
    if (url.empty()) return url;

    if (!url.empty() && url.back() == '/') {
        url.pop_back();
    }

    // 兼容用户仅配置 base url，例如 https://api.deepseek.com
    if (url.find("/chat/completions") == std::string::npos) {
        if (url.find("/v1") == std::string::npos) {
            url += "/v1/chat/completions";
        } else {
            url += "/chat/completions";
        }
    }
    return url;
}

bool ServerConfig::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        size_t pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string keyRaw = trim(line.substr(0, pos));
        std::string value = stripQuotes(trim(line.substr(pos + 1)));
        std::string key = toLower(keyRaw);

        // 网络配置（兼容 server.conf 与 .env）
        if (key == "server_ip") {
            ip = value;
        } else if (key == "server_port") {
            port = static_cast<uint16_t>(safeToInt(value, port));
        } else if (key == "io_threads") {
            ioThreads = safeToInt(value, ioThreads);
        } else if (key == "work_threads") {
            workThreads = safeToInt(value, workThreads);
        }

        // 数据库
        else if (key == "db_path") {
            dbPath = value;
        }

        // AI 配置（兼容 AI_* 与 DEEPSEEK_*）
        else if (key == "ai_api_key" || key == "deepseek_api_key") {
            aiApiKey = value;
        } else if (key == "ai_api_url") {
            aiApiUrl = normalizeAiUrl(value);
        } else if (key == "deepseek_base_url") {
            aiApiUrl = normalizeAiUrl(value);
        } else if (key == "ai_model" || key == "deepseek_model") {
            aiModel = value;
        }        else if (key == "ai_temperature") {
            try { aiTemperature = std::stod(value); } catch (...) {}
        }
        else if (key == "ai_max_tokens") {
            aiMaxTokens = safeToInt(value, aiMaxTokens);
        }
        // 聊天参数
        else if (key == "max_history_context") {
            maxHistoryContext = safeToInt(value, maxHistoryContext);
        }
    }

    file.close();
    return true;
}

void ServerConfig::print() const {
    std::cout << "========== 服务器配置 ==========" << std::endl;
    std::cout << "  监听地址:   " << ip << ":" << port << std::endl;
    std::cout << "  IO线程数:   " << ioThreads << std::endl;
    std::cout << "  工作线程数: " << workThreads << std::endl;
    std::cout << "  数据库路径: " << dbPath << std::endl;
    std::cout << "  AI模型:     " << aiModel << std::endl;
    std::cout << "  AI API:     " << aiApiUrl << std::endl;
    std::cout << "  API密钥:    " << (aiApiKey.empty() ? "(未配置)" : "****已配置") << std::endl;
    std::cout << "  温度:       " << aiTemperature << std::endl;
    std::cout << "  最大tokens: " << aiMaxTokens << std::endl;
    std::cout << "  历史上下文: " << maxHistoryContext << " 条" << std::endl;
    std::cout << "================================" << std::endl;
}
