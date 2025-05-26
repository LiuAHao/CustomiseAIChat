#ifndef AIServer_H
#define AIServer_H

#include<string>
#include<memory>
#include<functional>
#include"../net/TcpServer.h"
#include"../net/Timestamp.h"
#include<array>

class AIServer
{
public:
    AIServer(const std::string &ip,const uint16_t port,int subthreadnum,int workthreadnum);
    ~AIServer();
    void Start();
    void Stop();
    void OnMessage(spConnection conn, std::string message);

    // Base64编码方法
    static void Base64Encode(const std::string& input, std::string* output);

private:
    void HandleNewConnection(spConnection conn);
    void HandleClose(spConnection conn);
    void HandleError(spConnection conn);
    void HandleMessage(spConnection conn,std::string message);
    void HandleSendComplete(spConnection conn);
    void HandleRemove(int fd);
    // 发送HTTP请求到Python服务
    std::string SendHttpRequest(const std::string& base64_msg);

    TcpServer tcpserver_;
    ThreadPool workthreadpool_;
};

#endif