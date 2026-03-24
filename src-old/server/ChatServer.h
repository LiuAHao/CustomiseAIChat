#pragma once
#include "TcpServer.h"
#include "ThreadPool.h"
#include <jsoncpp/json/json.h>
#include <string>

class ChatServer {
public:
    ChatServer(const std::string& ip, uint16_t port, int ioThreads, int workThreads);
    ~ChatServer();

    void start();
    void stop();

private:
    void handleNewConnection(spConnection conn);
    void handleClose(spConnection conn);
    void handleError(spConnection conn);
    void handleMessage(spConnection conn, std::string message);
    void handleSendComplete(spConnection conn);
    void handleRemove(int fd);

    // 在工作线程中处理业务消息
    void processMessage(spConnection conn, std::string message);

    // 根据action分发到具体的Service
    std::string dispatchAction(const std::string& action, const Json::Value& request,
                               const std::string& token);

    // 构建错误响应
    std::string makeErrorResponse(int code, const std::string& message);

    TcpServer tcpserver_;
    ThreadPool workpool_;
};
