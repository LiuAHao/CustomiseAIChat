#include "AIService.h"
#include "../db/Database.h"
#include "../config/ServerConfig.h"
#include "../HttpClient.h"
#include <jsoncpp/json/json.h>
#include <iostream>

AIService& AIService::instance() {
    static AIService svc;
    return svc;
}

std::string AIService::sendMessage(int userId, int personaId, const std::string& message) {
    Json::FastWriter writer;

    if (message.empty()) {
        Json::Value resp;
        resp["code"] = -1;
        resp["message"] = "消息不能为空";
        return writer.write(resp);
    }

    // 获取人设信息
    Persona persona;
    if (!Database::instance().getPersona(personaId, userId, persona)) {
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
    Database::instance().addMessage(personaId, userId, "user", message);

    // 获取最近的聊天历史作为上下文
    auto history = Database::instance().getRecentMessages(
        personaId, userId, config.maxHistoryContext);

    // 调用AI API
    std::string aiReply = callAIAPI(persona.description, history, message);

    if (aiReply.empty()) {
        Json::Value resp;
        resp["code"] = -1;
        resp["message"] = "AI服务暂时不可用";
        return writer.write(resp);
    }

    // 保存AI回复到数据库
    Database::instance().addMessage(personaId, userId, "assistant", aiReply);

    Json::Value resp;
    resp["code"] = 0;
    resp["response"] = aiReply;
    resp["persona"] = persona.name;

    std::cout << "[AIService] 用户(" << userId << ") -> 人设(" << persona.name
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
        if (&msg == &history.back() && msg.sender == "user" && msg.content == userMessage) {
            continue;
        }
        Json::Value histMsg;
        histMsg["role"] = msg.sender;  // "user" 或 "assistant"
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
