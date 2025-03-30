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
    std::function<void(Connection*)> closecallback_;    //关闭连接
    std::function<void(Connection*)> errorcallback_;    //连接错误


public:
    Connection(EventLoop* loop,Socket* clientsock);
    ~Connection();

    int fd() const;
    std::string ip();
    uint16_t port();

    void closecallback();       //TCP断开的回调函数
    void errorcallback();           //TCP错误的回调函数

    void setclosecallback(std::function<void(Connection*)>fn);     //设置关闭连接事件的回调函数
    void seterrorcallback(std::function<void(Connection*)>fn);     //设置连接错误事件的回调函数
};