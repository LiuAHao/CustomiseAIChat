#include"AIServer.h"

AIServer::AIServer(const std::string &ip,const uint16_t port,int subthreadnum,int workthreadnum)
            :tcpserver_(ip,port,subthreadnum),workthreadpool_(workthreadnum,"WORKS")
{
    //业务关系什么，就设置什么回调函数
    tcpserver_.setnewconnectioncb(std::bind(&AIServer::HandleNewConnection,this,std::placeholders::_1));
    tcpserver_.setcloseconnectioncb(std::bind(&AIServer::HandleClose,this,std::placeholders::_1));
    tcpserver_.seterrorconnectioncb(std::bind(&AIServer::HandleError,this,std::placeholders::_1)); 
    tcpserver_.setonmessagecb(std::bind(&AIServer::HandleMessage,this,std::placeholders::_1,std::placeholders::_2));
    //tcpserver_.settimeoutcb(std::bind(&EchoServer::HandleEpollTimeOut,this,std::placeholders::_1));
    tcpserver_.setsendcompletecb(std::bind(&AIServer::HandleSendComplete,this,std::placeholders::_1));
   
}

AIServer::~AIServer()
{
    workthreadpool_.stop();
    //printf("工作线程已经停止\n");
    tcpserver_.stop();
}

void AIServer::Start()
{
    tcpserver_.start();
}

void AIServer::Stop()
{

}


void AIServer::HandleNewConnection(spConnection conn)
{
    printf("%s EchoServer new connection come in(fd=%d,ip=%s,port=%d).\n",Timestamp::now().tostring().c_str(),conn->fd(),conn->ip().data(),conn->port());  
}

void AIServer::HandleClose(spConnection conn)
{
    printf("%s EchoServer connection closed(fd=%d,ip=%s,port=%d).\n",Timestamp::now().tostring().c_str(),conn->fd(),conn->ip().data(),conn->port());  
}

void AIServer::HandleError(spConnection conn)
{
    //std::cout<<"EchoServer error connection.\n"; 
}

void AIServer::HandleMessage(spConnection conn,std::string message)
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

void AIServer::HandleSendComplete(spConnection conn)
{
    //std::cout<<"EchoServer send complete.\n"; 

} 

void AIServer::OnMessage(spConnection conn,std::string message)
{
    //printf("%s message (eventfd=%d):%s.\n",Timestamp::now().tostring().c_str(),conn->fd(),message.c_str());
    message = "reply" + message;    //回显业务
    conn->send(message.data(),message.size());
}