#pragma once
#include<functional>
#include"Socket.h"
#include"InetAddress.h"
#include"Channel.h"
#include"EventLoop.h"

class Connection
{
private:
    EventLoop* loop_;           //对应事件循环
    Socket* clientsock_;          //服务端用于监听的socket
    Channel* clientchannel_;    //对应的channel
public:
    Connection(EventLoop* loop,Socket* clientsock);
    ~Connection();

    int fd() const;
    std::string ip();
    uint16_t port();
};