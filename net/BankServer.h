#pragma once
#include"TcpServer.h"
#include"EventLoop.h"
#include"Connection.h"
#include"ThreadPool.h"

class UserInfo      //用户客户端信息（状态机）
{
private:
    int fd_;
    std::string ip_;
    bool login_ = false;
    std::string name_;

public:
    UserInfo(int fd,const std::string& ip):fd_(fd),ip_(ip){}
    void setLogin(bool login){login_ =login;}
    bool Login(){return login_;}
};


class BankServer
{
private:
    using spUserInfo = std::shared_ptr<UserInfo>;
    TcpServer tcpserver_;
    ThreadPool workthreadpool_;     //处理业务的工作线程池
    std::mutex mutex_;              //保护usermap_的互斥锁
    std::map<int,spUserInfo> usermap_;  //用户状态机
public:
    BankServer(const std::string &ip,const uint16_t port,int subthreadnum = 3,int workthreadnum = 5);
    ~BankServer();
    void Start();
    void Stop();

    void HandleNewConnection(spConnection conn);             //用于处理新客户端的请求
    void HandleClose(spConnection conn);             //关闭客户端的连接，在TcpServer类中回调此函数
    void HandleError(spConnection conn);             //客户端的错误连接，在TcpServer类中回调此函数
    void HandleMessage(spConnection conn,std::string message); //处理客户端的请求报文，在TcpServer中回调此函数
    void HandleSendComplete(spConnection conn);                //数据传送完毕后，在TcpServer类中回调此函数      
    //void HandleEpollTimeOut(EventLoop* loop);                //epoll_wait()超时，在TcpServer类中回调此函数    
    void OnMessage(spConnection conn,std::string message);        //处理客户端的请求报文，增加到线程池中
    void HandleRemove(int fd);
};