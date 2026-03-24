#pragma once

#include<map>
#include<memory>
#include<mutex>
#include"EventLoop.h"
#include"Socket.h"
#include"Channel.h"
#include"Acceptor.h"
#include"Connection.h"
#include"ThreadPool.h"

//TCP网络服务类
class TcpServer
{
private:
    std::unique_ptr<EventLoop> mainloop_;        //主事件循环，统一使用堆内存
    std::vector<std::unique_ptr<EventLoop>> subloops_;   //存放从事件循环的容器
    Acceptor acceptor_;                            //一个Tcpserver只有一个Acceptor对象，用栈对象
    int threadnum_;
    ThreadPool threadpool_;
    std::mutex mmutex_;         //保护conns_的互斥锁

    std::map<int,spConnection> conns_;
    std::function<void(spConnection)> newconnectioncb_;
    std::function<void(spConnection)> closeconnectioncb_;
    std::function<void(spConnection)> errorconnectioncb_;
    std::function<void(spConnection,std::string& message)> onmessagecb_;
    std::function<void(spConnection)> sendcompletecb_;
    std::function<void(EventLoop*)> timeoutcb_;                             //回调EchoServer类的回调函数
    std::function<void(int)> removeconnectioncb_;                             //回调上层业务的HanlleRemove


public:
    TcpServer(const std::string &ip,const uint16_t port,int threadnum = 3);
    ~TcpServer();
    void start();
    void stop();

    void newconnection(std::unique_ptr<Socket> clientsock);             //用于处理新客户端的请求
    void closeconnection(spConnection conn);             //关闭客户端的连接，在Connection类中回调此函数
    void errorconnection(spConnection conn);             //客户端的错误连接，在Connection类中回调此函数
    void onmessage(spConnection conn,std::string& message); //处理客户端的请求报文，在Connection中回调此函数
    void sendcomplete(spConnection conn);                //数据传送完毕后，在Connection类中回调此函数      
    void epolltimeout(EventLoop* loop);                //epoll_wait()超时，在EventLoop类中回调此函数      

    void setnewconnectioncb(std::function<void(spConnection)>fn);
    void setcloseconnectioncb(std::function<void(spConnection)>fn);
    void seterrorconnectioncb(std::function<void(spConnection)>fn);
    void setonmessagecb(std::function<void(spConnection,std::string& message)>fn);
    void setsendcompletecb(std::function<void(spConnection)>fn);
    void settimeoutcb(std::function<void(EventLoop*)>fn);           //设置回调函数
    void setremoveconnectioncb(std::function<void(int)>fn);           //设置回调函数

    void removeconn(int fd);         //删除TcpServer中的Connection连接


};