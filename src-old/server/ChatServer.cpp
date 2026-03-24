#include "ChatServer.h"
#include "service/UserService.h"
#include "service/ChatService.h"
#include "service/AIService.h"
#include "Timestamp.h"
#include <iostream>

ChatServer::ChatServer(const std::string& ip, uint16_t port, int ioThreads, int workThreads)
    : tcpserver_(ip, port, ioThreads), workpool_(workThreads, "WORK")
{
    tcpserver_.setnewconnectioncb(
        std::bind(&ChatServer::handleNewConnection, this, std::placeholders::_1));
    tcpserver_.setcloseconnectioncb(
        std::bind(&ChatServer::handleClose, this, std::placeholders::_1));
    tcpserver_.seterrorconnectioncb(
        std::bind(&ChatServer::handleError, this, std::placeholders::_1));
    tcpserver_.setonmessagecb(
        std::bind(&ChatServer::handleMessage, this, std::placeholders::_1, std::placeholders::_2));
    tcpserver_.setsendcompletecb(
        std::bind(&ChatServer::handleSendComplete, this, std::placeholders::_1));
    tcpserver_.setremoveconnectioncb(
        std::bind(&ChatServer::handleRemove, this, std::placeholders::_1));
}

ChatServer::~ChatServer() {
    workpool_.stop();
    tcpserver_.stop();
}

void ChatServer::start() {
    std::cout << "[ChatServer] 服务器已启动" << std::endl;
    tcpserver_.start();
}

void ChatServer::stop() {
    std::cout << "[ChatServer] 服务器正在停止..." << std::endl;
    workpool_.stop();
    tcpserver_.stop();
}

void ChatServer::handleNewConnection(spConnection conn) {
    printf("[%s] 新连接 fd=%d ip=%s port=%d\n",
           Timestamp::now().tostring().c_str(), conn->fd(), conn->ip().c_str(), conn->port());
}

void ChatServer::handleClose(spConnection conn) {
    printf("[%s] 连接断开 fd=%d ip=%s port=%d\n",
           Timestamp::now().tostring().c_str(), conn->fd(), conn->ip().c_str(), conn->port());
}

void ChatServer::handleError(spConnection conn) {
    printf("[%s] 连接错误 fd=%d ip=%s port=%d\n",
           Timestamp::now().tostring().c_str(), conn->fd(), conn->ip().c_str(), conn->port());
}

void ChatServer::handleMessage(spConnection conn, std::string message) {
    // 将业务处理分派到工作线程池，避免阻塞IO线程
    if (workpool_.size() == 0) {
        processMessage(conn, std::move(message));
    } else {
        workpool_.addtask(std::bind(&ChatServer::processMessage, this, conn, std::move(message)));
    }
}

void ChatServer::handleSendComplete(spConnection conn) {
    // 发送完成，可做日志记录
}

void ChatServer::handleRemove(int fd) {
    printf("[%s] 连接超时移除 fd=%d\n", Timestamp::now().tostring().c_str(), fd);
}

std::string ChatServer::makeErrorResponse(int code, const std::string& message) {
    Json::Value resp;
    resp["code"] = code;
    resp["message"] = message;
    Json::FastWriter writer;
    return writer.write(resp);
}

void ChatServer::processMessage(spConnection conn, std::string message) {
    // 解析JSON请求
    Json::Value request;
    Json::Reader reader;

    if (!reader.parse(message, request)) {
        std::string resp = makeErrorResponse(-1, "无效的JSON格式");
        conn->send(resp.data(), resp.size());
        return;
    }

    std::string action = request.get("action", "").asString();
    if (action.empty()) {
        std::string resp = makeErrorResponse(-1, "缺少action字段");
        conn->send(resp.data(), resp.size());
        return;
    }

    std::string token = request.get("token", "").asString();

    // 分发处理
    std::string result = dispatchAction(action, request, token);
    conn->send(result.data(), result.size());
}

std::string ChatServer::dispatchAction(const std::string& action, const Json::Value& request,
                                        const std::string& token) {
    // ===== 无需登录的操作 =====
    if (action == "register") {
        return UserService::instance().registerUser(
            request.get("username", "").asString(),
            request.get("password", "").asString(),
            request.get("nickname", "").asString()
        );
    }

    if (action == "login") {
        return UserService::instance().loginUser(
            request.get("username", "").asString(),
            request.get("password", "").asString()
        );
    }

    // ===== 以下操作需要登录 =====
    int userId = UserService::instance().getUserIdByToken(token);
    if (userId <= 0) {
        return makeErrorResponse(-2, "未登录或会话已过期，请先登录");
    }

    // --- 用户管理 ---
    if (action == "logout") {
        return UserService::instance().logoutUser(token);
    }
    if (action == "get_user_info") {
        return UserService::instance().getUserInfo(token);
    }
    if (action == "update_password") {
        return UserService::instance().updatePassword(
            token,
            request.get("oldPassword", "").asString(),
            request.get("newPassword", "").asString()
        );
    }
    if (action == "update_nickname") {
        return UserService::instance().updateNickname(
            token,
            request.get("nickname", "").asString()
        );
    }
    if (action == "delete_account") {
        return UserService::instance().deleteAccount(
            token,
            request.get("password", "").asString()
        );
    }

    // --- 人设管理 ---
    if (action == "create_persona") {
        return ChatService::instance().createPersona(
            userId,
            request.get("name", "").asString(),
            request.get("description", "").asString(),
            request.get("systemPrompt", "").asString(),
            request.get("avatar", "").asString()
        );
    }
    if (action == "delete_persona") {
        return ChatService::instance().deletePersona(
            userId,
            request.get("personaId", 0).asInt()
        );
    }
    if (action == "list_personas") {
        return ChatService::instance().listPersonas(userId);
    }
    if (action == "get_persona") {
        return ChatService::instance().getPersona(
            userId,
            request.get("personaId", 0).asInt()
        );
    }
    if (action == "update_persona") {
        return ChatService::instance().updatePersona(
            userId,
            request.get("personaId", 0).asInt(),
            request.get("name", "").asString(),
            request.get("description", "").asString(),
            request.get("systemPrompt", "").asString(),
            request.get("avatar", "").asString()
        );
    }

    // --- 会话管理 ---
    if (action == "create_conversation") {
        return ChatService::instance().createConversation(
            userId,
            request.get("personaId", 0).asInt(),
            request.get("title", "").asString()
        );
    }
    if (action == "delete_conversation") {
        return ChatService::instance().deleteConversation(
            userId,
            request.get("conversationId", 0).asInt()
        );
    }
    if (action == "list_conversations") {
        return ChatService::instance().listConversations(
            userId,
            request.get("limit", 50).asInt()
        );
    }
    if (action == "list_conversations_by_persona") {
        return ChatService::instance().listConversationsByPersona(
            userId,
            request.get("personaId", 0).asInt()
        );
    }
    if (action == "get_conversation") {
        return ChatService::instance().getConversation(
            userId,
            request.get("conversationId", 0).asInt()
        );
    }
    if (action == "update_conversation_title") {
        return ChatService::instance().updateConversationTitle(
            userId,
            request.get("conversationId", 0).asInt(),
            request.get("title", "").asString()
        );
    }

    // --- 聊天操作 ---
    if (action == "send_message") {
        return AIService::instance().sendMessage(
            userId,
            request.get("conversationId", 0).asInt(),
            request.get("message", "").asString()
        );
    }
    if (action == "greet") {
        return AIService::instance().sendGreeting(
            userId,
            request.get("conversationId", 0).asInt()
        );
    }
    if (action == "generate_desc") {
        return AIService::instance().generateDescription(
            userId,
            request.get("name", "").asString()
        );
    }
    if (action == "get_history") {
        return ChatService::instance().getHistory(
            userId,
            request.get("conversationId", 0).asInt(),
            request.get("limit", 50).asInt()
        );
    }
    if (action == "clear_history") {
        return ChatService::instance().clearHistory(
            userId,
            request.get("conversationId", 0).asInt()
        );
    }

    return makeErrorResponse(-1, "未知的操作: " + action);
}
