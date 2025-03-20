#pragma once
#include<functional>
#include"Socket.h"
#include"InetAddress.h"
#include"Channel.h"
#include"EventLoop.h"

class Acceptor
{
private:
    EventLoop* loop_;           //对应事件循环
    Socket* servsock_;          //服务端用于监听的socket
    Channel* acceptchannel_;    //对应的channel
public:
    Acceptor(EventLoop* loop,const std::string &ip,uint16_t port);
    ~Acceptor();

    void newconnection();           //用于处理新客户端的请求
};