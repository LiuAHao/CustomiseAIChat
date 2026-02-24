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

std::string ChatService::createPersona(int userId, const std::string& name, const std::string& description) {
    if (name.empty()) {
        return makeResponse(-1, "人设名称不能为空");
    }

    int personaId = Database::instance().createPersona(userId, name, description);
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
        item["createdAt"] = p.createdAt;
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
    resp["persona"]["createdAt"] = persona.createdAt;
    Json::FastWriter writer;
    return writer.write(resp);
}

std::string ChatService::updatePersona(int userId, int personaId,
                                        const std::string& name, const std::string& description) {
    if (name.empty()) {
        return makeResponse(-1, "人设名称不能为空");
    }

    if (!Database::instance().updatePersona(personaId, userId, name, description)) {
        return makeResponse(-1, "更新人设失败");
    }

    std::cout << "[ChatService] 用户(" << userId << ")更新人设(ID:" << personaId << ")" << std::endl;
    return makeResponse(0, "更新成功");
}

std::string ChatService::getHistory(int userId, int personaId, int limit) {
    if (limit <= 0) limit = 50;
    if (limit > 200) limit = 200;

    auto messages = Database::instance().getMessages(personaId, userId, limit);

    Json::Value resp;
    resp["code"] = 0;
    resp["messages"] = Json::Value(Json::arrayValue);

    for (const auto& msg : messages) {
        Json::Value item;
        item["id"] = msg.id;
        item["sender"] = msg.sender;
        item["content"] = msg.content;
        item["timestamp"] = msg.timestamp;
        resp["messages"].append(item);
    }

    Json::FastWriter writer;
    return writer.write(resp);
}

std::string ChatService::clearHistory(int userId, int personaId) {
    if (!Database::instance().clearMessages(personaId, userId)) {
        return makeResponse(-1, "清空聊天记录失败");
    }

    std::cout << "[ChatService] 用户(" << userId << ")清空人设(ID:" << personaId << ")聊天记录" << std::endl;
    return makeResponse(0, "聊天记录已清空");
}
