#pragma once
#include"../net/TcpServer.h"
#include"../net/EventLoop.h"
#include"../net/Connection.h"
#include"../net/ThreadPool.h"
#include <vector>
#include <string> // 添加 string 头文件
#include <json/json.h>

class AIServer {
private:
    TcpServer tcpserver_;
    ThreadPool workthreadpool_;
public:
    AIServer(const std::string &ip,const uint16_t port,int subthreadnum = 3,int workthreadnum = 5);
    ~AIServer();
    void Start();
    void Stop();

    void HandleNewConnection(spConnection conn);
    void HandleClose(spConnection conn);
    void HandleError(spConnection conn);
    void HandleMessage(spConnection conn,std::string message);
    void HandleSendComplete(spConnection conn);
    void OnMessage(spConnection conn,std::string message);
    std::string ExecPython(const char* cmd, const char* input);

    // 添加 Base64Encode 的静态成员函数声明
    static void Base64Encode(const std::string& input, std::string* output);
};
