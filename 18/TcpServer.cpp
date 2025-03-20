#include"TcpServer.h"

TcpServer::TcpServer(const std::string &ip,const uint16_t port)
{
    acceptor_ = new Acceptor(&loop_,ip,port);
    acceptor_->setnewconnectioncb(std::bind(&TcpServer::newconnection,this,std::placeholders::_1));
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
