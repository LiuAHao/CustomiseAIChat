#pragma once

#include"EventLoop.h"
#include"Socket.h"
#include"Channel.h"

class TcpServer
{
private:
    EventLoop loop_;        //一个Tcpserver可以有多个事件循环，单线程先用一个
public:
    TcpServer(const std::string &ip,const uint16_t port);
    ~TcpServer();
    void start();
};