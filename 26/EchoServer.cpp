#include"EchoServer.h"

EchoServer::EchoServer(const std::string &ip,const uint16_t port,int threadnum):tcpserver_(ip,port,threadnum)
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
    printf("EchoServer::HandleNewConnection thread is %d.\n",(int)syscall(SYS_gettid));

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
    printf("EchoServer::HandleMessage thread is %d.\n",(int)syscall(SYS_gettid));

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

