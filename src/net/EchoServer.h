#pragma once
#include"TcpServer.h"
#include"EventLoop.h"
#include"Connection.h"
#include"ThreadPool.h"

//回显服务器

class EchoServer
{
private:
    TcpServer tcpserver_;
    ThreadPool workthreadpool_;     //处理业务的工作线程池
public:
    EchoServer(const std::string &ip,const uint16_t port,int subthreadnum = 3,int workthreadnum = 5);
    ~EchoServer();
    void Start();
    void Stop();

    void HandleNewConnection(spConnection conn);             //用于处理新客户端的请求
    void HandleClose(spConnection conn);             //关闭客户端的连接，在TcpServer类中回调此函数
    void HandleError(spConnection conn);             //客户端的错误连接，在TcpServer类中回调此函数
    void HandleMessage(spConnection conn,std::string message); //处理客户端的请求报文，在TcpServer中回调此函数
    void HandleSendComplete(spConnection conn);                //数据传送完毕后，在TcpServer类中回调此函数      
    //void HandleEpollTimeOut(EventLoop* loop);                //epoll_wait()超时，在TcpServer类中回调此函数    
    void OnMessage(spConnection conn,std::string message);        //处理客户端的请求报文，增加到线程池中
};