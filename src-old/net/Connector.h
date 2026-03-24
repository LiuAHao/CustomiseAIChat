#pragma once
#include<functional>
#include<memory>
#include"Socket.h"
#include"InetAddress.h"
#include"Channel.h"
#include"EventLoop.h"

//连接器类，负责主动发起TCP连接（客户端使用）
//与Acceptor对应，Acceptor被动接受连接，Connector主动发起连接
class Connector
{
private:
    EventLoop* loop_;                       //对应的事件循环
    InetAddress servaddr_;                  //服务端地址
    int sockfd_;                            //用于连接的socket fd
    std::unique_ptr<Channel> connectchannel_;   //用于监视连接完成的channel
    bool connected_;                        //是否已经连接成功（fd已转移给Connection）

    std::function<void(std::unique_ptr<Socket>)> newconnectioncb_;  //连接成功的回调函数
    std::function<void()> errorconnectioncb_;                       //连接失败的回调函数

public:
    Connector(EventLoop* loop,const std::string &ip,uint16_t port);
    ~Connector();

    void start();           //发起非阻塞连接
    void handlewrite();     //连接完成的回调（EPOLLOUT触发）
    void handleerror();     //连接错误的回调

    void setnewconnectioncb(std::function<void(std::unique_ptr<Socket>)> fn);
    void seterrorconnectioncb(std::function<void()> fn);
};
