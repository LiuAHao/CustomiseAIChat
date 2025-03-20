#pragma once
#include<functional>
#include<memory>
#include"Socket.h"
#include"InetAddress.h"
#include"Channel.h"
#include"EventLoop.h"

class Acceptor
{
private:
    EventLoop* loop_;        //对应事件循环,无所有权只能使用常量引用
    Socket servsock_;          //服务端用于监听的socket
    Channel acceptchannel_;    //对应的channel
    std::function<void(std::unique_ptr<Socket>)> newconnectioncb_;      //处理新客户端连接的回调函数，新客户端一连接触发newconnect(),创建连接客户端程序
public:
    Acceptor(EventLoop* loop,const std::string &ip,uint16_t port);
    ~Acceptor();

    void newconnection();           //用于处理新客户端的请求

    void setnewconnectioncb(std::function<void(std::unique_ptr<Socket>)> fn);
};