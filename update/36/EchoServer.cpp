#include"EchoServer.h"

EchoServer::EchoServer(const std::string &ip,const uint16_t port,int subthreadnum,int workthreadnum)
            :tcpserver_(ip,port,subthreadnum),workthreadpool_(workthreadnum,"WORKS")
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
    workthreadpool_.stop();
    //printf("工作线程已经停止\n");
    tcpserver_.stop();
}

void EchoServer::Start()
{
    tcpserver_.start();
}

void EchoServer::Stop()
{

}


void EchoServer::HandleNewConnection(spConnection conn)
{
    printf("EchoServer new connection come in(fd=%d,ip=%s,port=%d).\n",conn->fd(),conn->ip().data(),conn->port());  
}

void EchoServer::HandleClose(spConnection conn)
{
    printf("EchoServer connection closed(fd=%d,ip=%s,port=%d).\n",conn->fd(),conn->ip().data(),conn->port());  
}

void EchoServer::HandleError(spConnection conn)
{
    //std::cout<<"EchoServer error connection.\n"; 
}

void EchoServer::HandleMessage(spConnection conn,std::string message)
{
    if(workthreadpool_.size() == 0)         //如果不需要工作线程（工作量小）
    {
        OnMessage(conn,message);
    }
    else
    {
        workthreadpool_.addtask(std::bind(&EchoServer::OnMessage,this,conn,message));
    }
   
}

void EchoServer::HandleSendComplete(spConnection conn)
{
    //std::cout<<"EchoServer send complete.\n"; 

} 

// void EchoServer::HandleEpollTimeOut(EventLoop* loop)
// {
//     std::cout<<"EchoServer timeout.\n"; 

// }

void EchoServer::OnMessage(spConnection conn,std::string message)
{
    message = "reply" + message;    //回显业务
    printf("message(fd=%d):%s\n",conn->fd(),message.data());
    conn->send(message.data(),message.size());
}