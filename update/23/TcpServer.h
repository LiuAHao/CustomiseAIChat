#pragma once

#include<map>
#include"EventLoop.h"
#include"Socket.h"
#include"Channel.h"
#include"Acceptor.h"
#include"Connection.h"

//TCP网络服务类
class TcpServer
{
private:
    EventLoop loop_;        //一个Tcpserver可以有多个事件循环，单线程先用一个
    Acceptor* acceptor_;    //一个Tcpserver只有一个Acceptor对象
    std::map<int,Connection*> conns_;
    std::function<void(Connection*)> newconnectioncb_;
    std::function<void(Connection*)> closeconnectioncb_;
    std::function<void(Connection*)> errorconnectioncb_;
    std::function<void(Connection*,std::string &message)> onmessagecb_;
    std::function<void(Connection*)> sendcompletecb_;
    std::function<void(EventLoop*)> timeoutcb_;                             //回调EchoServer类的回调函数

public:
    TcpServer(const std::string &ip,const uint16_t port);
    ~TcpServer();
    void start();

    void newconnection(Socket* clientsock);             //用于处理新客户端的请求
    void closeconnection(Connection* conn);             //关闭客户端的连接，在Connection类中回调此函数
    void errorconnection(Connection* conn);             //客户端的错误连接，在Connection类中回调此函数
    void onmessage(Connection* conn,std::string message); //处理客户端的请求报文，在Connection中回调此函数
    void sendcomplete(Connection* conn);                //数据传送完毕后，在Connection类中回调此函数      
    void epolltimeout(EventLoop* loop);                //epoll_wait()超时，在EventLoop类中回调此函数      

    void setnewconnectioncb(std::function<void(Connection*)>fn);
    void setcloseconnectioncb(std::function<void(Connection*)>fn);
    void seterrorconnectioncb(std::function<void(Connection*)>fn);
    void setonmessagecb(std::function<void(Connection*,std::string)>fn);
    void setsendcompletecb(std::function<void(Connection*)>fn);
    void settimeoutcb(std::function<void(EventLoop*)>fn);           //设置回调函数


};