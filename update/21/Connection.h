#pragma once
#include<functional>
#include"Socket.h"
#include"InetAddress.h"
#include"Channel.h"
#include"EventLoop.h"
#include"Buffer.h"

class Connection
{
private:
    EventLoop* loop_;           //对应事件循环
    Socket* clientsock_;          //服务端用于监听的socket
    Channel* clientchannel_;    //对应的channel
    Buffer inputbuffer_;          //接收缓冲区
    Buffer outputbuffer_;         //发送缓冲区

    std::function<void(Connection*)> closecallback_;    //关闭连接
    std::function<void(Connection*)> errorcallback_;    //连接错误
    std::function<void(Connection*,std::string)> onmessagecallback_;    //处理客户端返回报文 

public:
    Connection(EventLoop* loop,Socket* clientsock);
    ~Connection();

    int fd() const;
    std::string ip();
    uint16_t port();

    void onmessage();
    void closecallback();       //TCP断开的回调函数
    void errorcallback();           //TCP错误的回调函数
    void writecallback();       //写入数据的回调函数

    void setclosecallback(std::function<void(Connection*)>fn);     //设置关闭连接事件的回调函数
    void seterrorcallback(std::function<void(Connection*)>fn);     //设置连接错误事件的回调函数
    void setonmessagecallback(std::function<void(Connection*,std::string)>fn);     //设置连处理客户端返回报文事件的回调函数

    void send(const char* data,size_t size);
};