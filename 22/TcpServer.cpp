#include"TcpServer.h"

TcpServer::TcpServer(const std::string &ip,const uint16_t port)
{
    acceptor_ = new Acceptor(&loop_,ip,port);
    acceptor_->setnewconnectioncb(std::bind(&TcpServer::newconnection,this,std::placeholders::_1));
    loop_.setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout,this,std::placeholders::_1));
}

TcpServer::~TcpServer()
{
    delete acceptor_;
    //释放全部Connection的对象
    for(auto &aa:conns_){
        delete aa.second;
    }
}

void TcpServer::start()
{
    loop_.run();
}

void TcpServer::newconnection(Socket* clientsock)           //用于处理新客户端的请求
{
    Connection* conn = new Connection(&loop_,clientsock);
    conn->setclosecallback(std::bind(&TcpServer::closeconnection,this,std::placeholders::_1));
    conn->seterrorcallback(std::bind(&TcpServer::errorconnection,this,std::placeholders::_1));
    conn->setonmessagecallback(std::bind(&TcpServer::onmessage,this,std::placeholders::_1,std::placeholders::_2));
    conn->setsendcompletecallback(std::bind(&TcpServer::sendcomplete,this,std::placeholders::_1));

    printf("accept client(fd=%d,ip=%s,port=%d) ok.\n",conn->fd(),conn->ip().c_str(),conn->port());

    conns_[conn->fd()] = conn;      //把conn放入map容器中
}

void TcpServer::closeconnection(Connection* conn)
{
    printf("client(eventfd=%d)disconnected.\n",conn->fd());
    //Socket的析构函数会关闭fd,无需close(conn->fd());
    conns_.erase(conn->fd());
    delete conns_[conn->fd()];
}

void TcpServer::errorconnection(Connection* conn)
{
    printf("client(eventfd=%d)error.\n",conn->fd());
    conns_.erase(conn->fd());
    delete conns_[conn->fd()];
}

void TcpServer::onmessage(Connection* conn,std::string message)
{
    message = "reply" + message;

    int len = message.size();
    std::string tmpbuf((char*)&len,4);
    tmpbuf.append(message);

    conn->send(tmpbuf.data(),tmpbuf.size());
}

void TcpServer::sendcomplete(Connection* conn)
{
    printf("send complete.\n");
    //根据需求，可增加代码
}      

void TcpServer::epolltimeout(EventLoop* loop)
{
    printf("epoll_wait() timeout.\n");
    //根据需求，可增加代码

}    
