#include"EchoServer.h"

// class EchoServer
// {
// private:
//     TcpServer tcpserver_;
// public:
//     EchoServer(const std::string &ip,const uint16_t port);
//     ~EchoServer();
//     void Start();

//     void HandleNewConnection(Socket* clientsock);             //用于处理新客户端的请求
//     void HandleCloseConnection(Connection* conn);             //关闭客户端的连接，在Connection类中回调此函数
//     void HandleErrorConnection(Connection* conn);             //客户端的错误连接，在Connection类中回调此函数
//     void HandleMessage(Connection* conn,std::string message); //处理客户端的请求报文，在Connection中回调此函数
//     void HandleSendComplete(Connection* conn);                //数据传送完毕后，在Connection类中回调此函数      
//     void HandleEpollTimeOut(EventLoop* loop);                //epoll_wait()超时，在EventLoop类中回调此函数      
// };

EchoServer::EchoServer(const std::string &ip,const uint16_t port):tcpserver_(ip,port)
{
    //业务关系什么，就设置什么回调函数
    tcpserver_.setnewconnectioncb(std::bind(&EchoServer::HandleNewConnection,this,std::placeholders::_1));
    tcpserver_.setcloseconnectioncb(std::bind(&EchoServer::HandleClose,this,std::placeholders::_1));
    tcpserver_.seterrorconnectioncb(std::bind(&EchoServer::HandleError,this,std::placeholders::_1)); 
    tcpserver_.setonmessagecb(std::bind(&EchoServer::HandleMessage,this,std::placeholders::_1,std::placeholders::_2));
    //tcpserver_.settimeoutcb(std::bind(&EchoServer::HandleEpollTimeOut,this,std::placeholders::_1));
    tcpserver_.setsendcompletecb(std::bind(&EchoServer::HandleSendComplete,this,std::placeholders::_1));
   

}

EchoServer::~EchoServer()
{

}

void EchoServer::Start()
{
    tcpserver_.start();
}

void EchoServer::HandleNewConnection(Connection* connect)
{
    std::cout<<"EchoServer new connection come in.\n"; 
    
}

void EchoServer::HandleClose(Connection* conn)
{
    std::cout<<"EchoServer close connection.\n"; 

}

void EchoServer::HandleError(Connection* conn)
{
    std::cout<<"EchoServer error connection.\n"; 

}

void EchoServer::HandleMessage(Connection* conn,std::string message)
{
    message = "reply" + message;    //回显业务
    
    conn->send(message.data(),message.size());   
}

void EchoServer::HandleSendComplete(Connection* conn)
{
    std::cout<<"EchoServer send complete.\n"; 

} 

// void EchoServer::HandleEpollTimeOut(EventLoop* loop)
// {
//     std::cout<<"EchoServer timeout.\n"; 

// }

