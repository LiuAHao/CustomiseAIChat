#include "AIService.h"
#include "../db/Database.h"
#include "../config/ServerConfig.h"
#include "../HttpClient.h"
#include "../model/Conversation.h"
#include "../model/Persona.h"
#include <jsoncpp/json/json.h>
#include <iostream>
#include <ctime>

AIService& AIService::instance() {
    static AIService svc;
    return svc;
}

std::string AIService::sendMessage(int userId, int conversationId, const std::string& message) {
    Json::FastWriter writer;

    if (message.empty()) {
        Json::Value resp;
        resp["code"] = -1;
        resp["message"] = "消息不能为空";
        return writer.write(resp);
    }

    // 校验会话归属并获取会话信息
    Conversation conv;
    if (!Database::instance().getConversation(conversationId, userId, conv)) {
        Json::Value resp;
        resp["code"] = -1;
        resp["message"] = "会话不存在或无权限";
        return writer.write(resp);
    }

    // 通过会话关联的人设ID获取人设信息
    Persona persona;
    if (!Database::instance().getPersona(conv.personaId, userId, persona)) {
        Json::Value resp;
        resp["code"] = -1;
        resp["message"] = "人设不存在";
        return writer.write(resp);
    }

    // 检查API密钥配置
    const auto& config = ServerConfig::instance();
    if (config.aiApiKey.empty()) {
        Json::Value resp;
        resp["code"] = -1;
        resp["message"] = "AI服务未配置API密钥";
        return writer.write(resp);
    }

    // 保存用户消息到数据库
    Database::instance().addMessage(conversationId, "user", message);

    // 获取最近的聊天历史作为上下文
    auto history = Database::instance().getRecentMessages(
        conversationId, config.maxHistoryContext);

    // 使用人设的 systemPrompt 作为系统提示
    std::string systemPrompt = persona.systemPrompt.empty()
        ? persona.description : persona.systemPrompt;

    // 调用AI API
    std::string aiReply = callAIAPI(systemPrompt, history, message);

    if (aiReply.empty()) {
        Json::Value resp;
        resp["code"] = -1;
        resp["message"] = "AI服务暂时不可用";
        return writer.write(resp);
    }

    // 保存AI回复到数据库
    Database::instance().addMessage(conversationId, "assistant", aiReply);

    // 如果是会话的第一条消息，自动用用户消息的前20字作为会话标题
    if (conv.messageCount == 0) {
        std::string autoTitle = message.substr(0, 60);  // UTF-8截取前60字节
        if (message.size() > 60) autoTitle += "...";
        Database::instance().updateConversationTitle(conversationId, userId, autoTitle);
    }

    Json::Value resp;
    resp["code"] = 0;
    resp["reply"] = aiReply;
    resp["persona"] = persona.name;
    resp["conversationId"] = conversationId;

    // 返回时间戳供前端显示（统一使用 UTC）
    {
        time_t now = time(nullptr);
        struct tm tm_info;
        gmtime_r(&now, &tm_info);
        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);
        resp["timestamp"] = std::string(buf);
    }

    std::cout << "[AIService] 用户(" << userId << ") -> 会话(" << conversationId
              << ") 人设(" << persona.name
              << ") 消息长度:" << message.size() << " 回复长度:" << aiReply.size() << std::endl;

    return writer.write(resp);
}

std::string AIService::callAIAPI(const std::string& systemPrompt,
                                  const std::vector<ChatMessage>& history,
                                  const std::string& userMessage) {
    const auto& config = ServerConfig::instance();

    // 构建请求body
    Json::Value requestBody;
    requestBody["model"] = config.aiModel;
    requestBody["stream"] = false;
    requestBody["temperature"] = config.aiTemperature;
    requestBody["max_tokens"] = config.aiMaxTokens;

    Json::Value messages(Json::arrayValue);

    // 系统提示（人设描述）
    if (!systemPrompt.empty()) {
        Json::Value sysMsg;
        sysMsg["role"] = "system";
        sysMsg["content"] = systemPrompt;
        messages.append(sysMsg);
    }

    // 添加聊天历史上下文（排除最后一条，因为最后一条是当前用户消息）
    for (const auto& msg : history) {
        // 跳过最后一条（就是刚保存的用户消息）
        if (&msg == &history.back() && msg.role == "user" && msg.content == userMessage) {
            continue;
        }
        Json::Value histMsg;
        histMsg["role"] = msg.role;  // "user" 或 "assistant"
        histMsg["content"] = msg.content;
        messages.append(histMsg);
    }

    // 当前用户消息
    Json::Value userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userMessage;
    messages.append(userMsg);

    requestBody["messages"] = messages;

    Json::FastWriter jsonWriter;
    std::string body = jsonWriter.write(requestBody);

    // 构建请求头
    std::map<std::string, std::string> headers;
    headers["Authorization"] = "Bearer " + config.aiApiKey;

    // 发送HTTP请求
    std::string response = HttpClient::instance().post(
        config.aiApiUrl, body, "application/json", headers);

    // 解析响应
    Json::Value responseJson;
    Json::Reader reader;
    if (!reader.parse(response, responseJson)) {
        std::cerr << "[AIService] 解析AI API响应失败" << std::endl;
        return "";
    }

    // 检查是否有错误
    if (responseJson.isMember("error")) {
        std::string errMsg = responseJson["error"].isObject()
            ? responseJson["error"].get("message", "未知错误").asString()
            : responseJson["error"].asString();
        std::cerr << "[AIService] AI API错误: " << errMsg << std::endl;
        return "";
    }

    // 提取AI回复内容
    if (responseJson.isMember("choices") && responseJson["choices"].isArray()
        && responseJson["choices"].size() > 0) {
        return responseJson["choices"][0]["message"]["content"].asString();
    }

    std::cerr << "[AIService] AI API响应格式异常" << std::endl;
    return "";
}

std::string AIService::sendGreeting(int userId, int conversationId) {
    Json::FastWriter writer;

    Conversation conv;
    if (!Database::instance().getConversation(conversationId, userId, conv)) {
        Json::Value resp;
        resp["code"] = -1;
        resp["message"] = "会话不存在或无权限";
        return writer.write(resp);
    }

    Persona persona;
    if (!Database::instance().getPersona(conv.personaId, userId, persona)) {
        Json::Value resp;
        resp["code"] = -1;
        resp["message"] = "人设不存在";
        return writer.write(resp);
    }

    const auto& config = ServerConfig::instance();
    std::string sysPrompt = persona.systemPrompt.empty() ? persona.description : persona.systemPrompt;

    std::string greetInstruction =
        "请以你的人设角色主动向用户打一声招呼，自然友好地介绍一下自己和能力，50字内即可。";

    std::string aiReply;
    if (!config.aiApiKey.empty()) {
        std::vector<ChatMessage> empty;
        aiReply = callAIAPI(sysPrompt, empty, greetInstruction);
    }

    // 如果AI调用失败，使用默认招呼语
    if (aiReply.empty()) {
        aiReply = "你好！我是" + persona.name;
        if (!persona.description.empty())
            aiReply += "，" + persona.description;
        aiReply += " 有什么可以帮助你的吗？";
    }

    Database::instance().addMessage(conversationId, "assistant", aiReply);

    Json::Value resp;
    resp["code"] = 0;
    resp["reply"] = aiReply;
    {
        time_t now = time(nullptr);
        struct tm tm_info;
        gmtime_r(&now, &tm_info);
        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);
        resp["timestamp"] = std::string(buf);
    }

    std::cout << "[AIService] 人设(" << persona.name << ") 向会话(" << conversationId << ")发送招呼" << std::endl;
    return writer.write(resp);
}

std::string AIService::generateDescription(int userId, const std::string& personaName) {
    Json::FastWriter writer;

    if (personaName.empty()) {
        Json::Value resp;
        resp["code"] = -1;
        resp["message"] = "人设名称不能为空";
        return writer.write(resp);
    }

    const auto& config = ServerConfig::instance();
    if (config.aiApiKey.empty()) {
        Json::Value resp;
        resp["code"] = -1;
        resp["message"] = "AI服务未配置API密钥";
        return writer.write(resp);
    }

    // 用系统提示告诉AI生成人设描述
    std::string systemPrompt =
        "你是一个AI角色设计专家。用户会给你一个角色名称，"
        "你需要为它写一段简洁生动的角色描述（50-100字），"
        "描述该角色的性格特点、擅长领域和交互风格。"
        "直接输出描述文本，不要用引号包裹，不要加任何前缀。";

    std::vector<ChatMessage> empty;
    std::string aiReply = callAIAPI(systemPrompt, empty,
        "请为名为「" + personaName + "」的AI角色生成描述。");

    if (aiReply.empty()) {
        Json::Value resp;
        resp["code"] = -1;
        resp["message"] = "AI生成描述失败，请稍后重试";
        return writer.write(resp);
    }

    Json::Value resp;
    resp["code"] = 0;
    resp["description"] = aiReply;

    std::cout << "[AIService] 用户(" << userId << ") AI生成人设描述: "
              << personaName << " -> " << aiReply.size() << "字" << std::endl;
    return writer.write(resp);
}
