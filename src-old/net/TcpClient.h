#pragma once
#include<functional>
#include<memory>
#include"EventLoop.h"
#include"Connection.h"
#include"Connector.h"

//TCP客户端类，使用Connector主动发起连接，复用Connection进行异步通信
//与TcpServer对应，TcpServer管理被动连接，TcpClient管理主动连接
class TcpClient
{
private:
    EventLoop loop_;                //客户端事件循环（单线程）
    Connector connector_;           //连接器，负责发起连接
    spConnection conn_;             //连接成功后创建的Connection对象

    std::function<void(spConnection)> connectioncb_;                        //连接成功的回调
    std::function<void(spConnection)> closecb_;                             //连接断开的回调
    std::function<void(spConnection)> errorcb_;                             //连接错误的回调
    std::function<void(spConnection,std::string&)> messagecb_;              //收到消息的回调
    std::function<void(spConnection)> sendcompletecb_;                      //发送完毕的回调

public:
    TcpClient(const std::string &ip,uint16_t port);
    ~TcpClient();

    void start();           //发起连接并启动事件循环（阻塞）
    void stop();            //停止事件循环

    bool send(const char* data,size_t size);        //发送数据（线程安全）
    spConnection connection();                      //获取当前连接

    void setconnectioncb(std::function<void(spConnection)> fn);
    void setclosecb(std::function<void(spConnection)> fn);
    void seterrorcb(std::function<void(spConnection)> fn);
    void setmessagecb(std::function<void(spConnection,std::string&)> fn);
    void setsendcompletecb(std::function<void(spConnection)> fn);

private:
    void newconnection(std::unique_ptr<Socket> clientsock);     //连接成功后创建Connection
    void closeconnection(spConnection conn);                    //连接断开回调
    void errorconnection(spConnection conn);                    //连接错误回调
    void onmessage(spConnection conn,std::string& message);     //收到消息回调
    void sendcomplete(spConnection conn);                       //发送完毕回调
    void connecterror();                                        //连接建立失败回调
};
