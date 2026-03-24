#include "ChatService.h"
#include "../db/Database.h"
#include <jsoncpp/json/json.h>
#include <iostream>

ChatService& ChatService::instance() {
    static ChatService svc;
    return svc;
}

std::string ChatService::makeResponse(int code, const std::string& message) {
    Json::Value resp;
    resp["code"] = code;
    resp["message"] = message;
    Json::FastWriter writer;
    return writer.write(resp);
}

// ===== 人设管理 =====

std::string ChatService::createPersona(int userId, const std::string& name,
                                        const std::string& description,
                                        const std::string& systemPrompt,
                                        const std::string& avatar) {
    if (name.empty()) {
        return makeResponse(-1, "人设名称不能为空");
    }

    // systemPrompt 为空时，使用 description 作为兜底
    std::string prompt = systemPrompt.empty() ? description : systemPrompt;

    int personaId = Database::instance().createPersona(userId, name, description, prompt, avatar);
    if (personaId < 0) {
        return makeResponse(-1, "创建人设失败");
    }

    Json::Value resp;
    resp["code"] = 0;
    resp["message"] = "创建成功";
    resp["personaId"] = personaId;
    Json::FastWriter writer;

    std::cout << "[ChatService] 用户(" << userId << ")创建人设: " << name << " (ID:" << personaId << ")" << std::endl;
    return writer.write(resp);
}

std::string ChatService::deletePersona(int userId, int personaId) {
    if (!Database::instance().deletePersona(personaId, userId)) {
        return makeResponse(-1, "删除人设失败");
    }

    std::cout << "[ChatService] 用户(" << userId << ")删除人设(ID:" << personaId << ")" << std::endl;
    return makeResponse(0, "删除成功");
}

std::string ChatService::listPersonas(int userId) {
    auto personas = Database::instance().getPersonasByUser(userId);

    Json::Value resp;
    resp["code"] = 0;
    resp["personas"] = Json::Value(Json::arrayValue);

    for (const auto& p : personas) {
        Json::Value item;
        item["id"] = p.id;
        item["name"] = p.name;
        item["description"] = p.description;
        item["systemPrompt"] = p.systemPrompt;
        item["avatar"] = p.avatar;
        item["createdAt"] = p.createdAt;
        item["updatedAt"] = p.updatedAt;
        resp["personas"].append(item);
    }

    Json::FastWriter writer;
    return writer.write(resp);
}

std::string ChatService::getPersona(int userId, int personaId) {
    Persona persona;
    if (!Database::instance().getPersona(personaId, userId, persona)) {
        return makeResponse(-1, "人设不存在");
    }

    Json::Value resp;
    resp["code"] = 0;
    resp["persona"]["id"] = persona.id;
    resp["persona"]["name"] = persona.name;
    resp["persona"]["description"] = persona.description;
    resp["persona"]["systemPrompt"] = persona.systemPrompt;
    resp["persona"]["avatar"] = persona.avatar;
    resp["persona"]["createdAt"] = persona.createdAt;
    resp["persona"]["updatedAt"] = persona.updatedAt;
    Json::FastWriter writer;
    return writer.write(resp);
}

std::string ChatService::updatePersona(int userId, int personaId,
                                        const std::string& name, const std::string& description,
                                        const std::string& systemPrompt, const std::string& avatar) {
    if (name.empty()) {
        return makeResponse(-1, "人设名称不能为空");
    }

    std::string prompt = systemPrompt.empty() ? description : systemPrompt;

    if (!Database::instance().updatePersona(personaId, userId, name, description, prompt, avatar)) {
        return makeResponse(-1, "更新人设失败");
    }

    std::cout << "[ChatService] 用户(" << userId << ")更新人设(ID:" << personaId << ")" << std::endl;
    return makeResponse(0, "更新成功");
}

// ===== 会话管理 =====

std::string ChatService::createConversation(int userId, int personaId, const std::string& title) {
    // 校验人设归属
    if (!Database::instance().isPersonaOwner(personaId, userId)) {
        return makeResponse(-1, "人设不存在或无权限");
    }

    std::string convTitle = title.empty() ? "新会话" : title;
    int convId = Database::instance().createConversation(userId, personaId, convTitle);
    if (convId < 0) {
        return makeResponse(-1, "创建会话失败");
    }

    Json::Value resp;
    resp["code"] = 0;
    resp["message"] = "创建会话成功";
    resp["conversationId"] = convId;
    Json::FastWriter writer;

    std::cout << "[ChatService] 用户(" << userId << ")在人设(" << personaId
              << ")下创建会话(ID:" << convId << ")" << std::endl;
    return writer.write(resp);
}

std::string ChatService::deleteConversation(int userId, int conversationId) {
    if (!Database::instance().deleteConversation(conversationId, userId)) {
        return makeResponse(-1, "删除会话失败");
    }

    std::cout << "[ChatService] 用户(" << userId << ")删除会话(ID:" << conversationId << ")" << std::endl;
    return makeResponse(0, "删除会话成功");
}

std::string ChatService::listConversations(int userId, int limit) {
    if (limit <= 0) limit = 50;
    if (limit > 200) limit = 200;

    auto convs = Database::instance().getConversationsByUser(userId, limit);

    Json::Value resp;
    resp["code"] = 0;
    resp["conversations"] = Json::Value(Json::arrayValue);

    for (const auto& c : convs) {
        Json::Value item;
        item["id"] = c.id;
        item["personaId"] = c.personaId;
        item["title"] = c.title;
        item["messageCount"] = c.messageCount;
        item["createdAt"] = c.createdAt;
        item["updatedAt"] = c.updatedAt;
        resp["conversations"].append(item);
    }

    Json::FastWriter writer;
    return writer.write(resp);
}

std::string ChatService::listConversationsByPersona(int userId, int personaId) {
    // 校验人设归属
    if (!Database::instance().isPersonaOwner(personaId, userId)) {
        return makeResponse(-1, "人设不存在或无权限");
    }

    auto convs = Database::instance().getConversationsByPersona(userId, personaId);

    Json::Value resp;
    resp["code"] = 0;
    resp["conversations"] = Json::Value(Json::arrayValue);

    for (const auto& c : convs) {
        Json::Value item;
        item["id"] = c.id;
        item["personaId"] = c.personaId;
        item["title"] = c.title;
        item["messageCount"] = c.messageCount;
        item["createdAt"] = c.createdAt;
        item["updatedAt"] = c.updatedAt;
        resp["conversations"].append(item);
    }

    Json::FastWriter writer;
    return writer.write(resp);
}

std::string ChatService::getConversation(int userId, int conversationId) {
    Conversation conv;
    if (!Database::instance().getConversation(conversationId, userId, conv)) {
        return makeResponse(-1, "会话不存在");
    }

    Json::Value resp;
    resp["code"] = 0;
    resp["conversation"]["id"] = conv.id;
    resp["conversation"]["personaId"] = conv.personaId;
    resp["conversation"]["title"] = conv.title;
    resp["conversation"]["messageCount"] = conv.messageCount;
    resp["conversation"]["createdAt"] = conv.createdAt;
    resp["conversation"]["updatedAt"] = conv.updatedAt;
    Json::FastWriter writer;
    return writer.write(resp);
}

std::string ChatService::updateConversationTitle(int userId, int conversationId,
                                                  const std::string& title) {
    if (title.empty()) {
        return makeResponse(-1, "会话标题不能为空");
    }

    if (!Database::instance().updateConversationTitle(conversationId, userId, title)) {
        return makeResponse(-1, "更新会话标题失败");
    }

    return makeResponse(0, "更新成功");
}

// ===== 聊天记录 =====

std::string ChatService::getHistory(int userId, int conversationId, int limit) {
    if (limit <= 0) limit = 50;
    if (limit > 200) limit = 200;

    // 校验会话归属
    if (!Database::instance().isConversationOwner(conversationId, userId)) {
        return makeResponse(-1, "会话不存在或无权限");
    }

    auto messages = Database::instance().getMessages(conversationId, limit);

    Json::Value resp;
    resp["code"] = 0;
    resp["messages"] = Json::Value(Json::arrayValue);

    for (const auto& msg : messages) {
        Json::Value item;
        item["id"] = msg.id;
        item["role"] = msg.role;
        item["content"] = msg.content;
        item["timestamp"] = msg.timestamp;
        resp["messages"].append(item);
    }

    Json::FastWriter writer;
    return writer.write(resp);
}

std::string ChatService::clearHistory(int userId, int conversationId) {
    if (!Database::instance().clearMessages(conversationId, userId)) {
        return makeResponse(-1, "清空聊天记录失败");
    }

    std::cout << "[ChatService] 用户(" << userId << ")清空会话(ID:" << conversationId << ")聊天记录" << std::endl;
    return makeResponse(0, "聊天记录已清空");
}
