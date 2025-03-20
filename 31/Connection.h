#pragma once
#include<functional>
#include<memory>
#include<atomic>
#include"Socket.h"
#include"InetAddress.h"
#include"Channel.h"
#include"EventLoop.h"
#include"Buffer.h"
class Connection;
using spConnection = std::shared_ptr<Connection>;   

class Connection:public std::enable_shared_from_this<Connection>        //继智能指针模版类，让this为智能指针类型
{
private:
    EventLoop* loop_;           //对应事件循环
    std::unique_ptr<Socket> clientsock_;          //服务端用于监听的socket,生命周期由Connection来管理
    std::unique_ptr<Channel> clientchannel_;    //对应的channel,一个服务程序有很多个Connection，用堆内存
    Buffer inputbuffer_;          //接收缓冲区
    Buffer outputbuffer_;         //发送缓冲区
    std::atomic_bool disconnect_;   //客户端连接是否已经断开

    std::function<void(spConnection)> closecallback_;    //关闭连接
    std::function<void(spConnection)> errorcallback_;    //连接错误
    std::function<void(spConnection,std::string&)> onmessagecallback_;    //处理客户端返回报文 
    std::function<void(spConnection)> sendcompletecallback_;    //传送完毕


public:
    Connection(EventLoop* loop,std::unique_ptr<Socket> clientsock);
    ~Connection();

    int fd() const;
    std::string ip();
    uint16_t port();

    void onmessage();
    void closecallback();       //TCP断开的回调函数
    void errorcallback();           //TCP错误的回调函数
    void writecallback();       //写入数据的回调函数

    void setclosecallback(std::function<void(spConnection)>fn);     //设置关闭连接事件的回调函数
    void seterrorcallback(std::function<void(spConnection)>fn);     //设置连接错误事件的回调函数
    void setonmessagecallback(std::function<void(spConnection,std::string&)>fn);     //设置连处理客户端返回报文事件的回调函数
    void setsendcompletecallback(std::function<void(spConnection)>fn);     //设置连处理客户端返回报文事件的回调函数

    void send(const char* data,size_t size);
};