#include"TcpServer.h"

TcpServer::TcpServer(const std::string &ip,const uint16_t port,int threadnum):threadnum_(threadnum)
{
    mainloop_ = new EventLoop;    
    mainloop_->setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout,this,std::placeholders::_1));

    acceptor_ = new Acceptor(mainloop_,ip,port);
    acceptor_->setnewconnectioncb(std::bind(&TcpServer::newconnection,this,std::placeholders::_1));

    threadpool_ = new ThreadPool(threadnum_,"IO");   //创建线程池
    for(int ii = 0; ii < threadnum_; ii++)
    {
        subloops_.push_back(new EventLoop);
        subloops_[ii]->setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout,this,std::placeholders::_1));
        threadpool_ -> addtask(std::bind(&EventLoop::run,subloops_[ii]));   //bind函数绑定特定类实例的成员函数，1为函数指针，2为对象实例
    }

}

TcpServer::~TcpServer()
{
    delete acceptor_;
    delete mainloop_;
    
    //释放从事件循环
    for(auto &aa:subloops_){
        delete aa;
    }
    delete threadpool_;
}

void TcpServer::start()
{
    mainloop_->run();
}

void TcpServer::newconnection(Socket* clientsock)           //用于处理新客户端的请求
{
    spConnection conn(new Connection(subloops_[clientsock->fd() % threadnum_],clientsock));     //随机分配到从事件循环
    conn->setclosecallback(std::bind(&TcpServer::closeconnection,this,std::placeholders::_1));
    conn->seterrorcallback(std::bind(&TcpServer::errorconnection,this,std::placeholders::_1));
    conn->setonmessagecallback(std::bind(&TcpServer::onmessage,this,std::placeholders::_1,std::placeholders::_2));
    conn->setsendcompletecallback(std::bind(&TcpServer::sendcomplete,this,std::placeholders::_1));

    //printf("accept client(fd=%d,ip=%s,port=%d) ok.\n",conn->fd(),conn->ip().c_str(),conn->port());

    conns_[conn->fd()] = conn;      //把conn放入map容器中

    if(newconnectioncb_)newconnectioncb_(conn);

}

void TcpServer::closeconnection(spConnection conn)
{
    if(closeconnectioncb_)closeconnectioncb_(conn);
    //printf("client(eventfd=%d)disconnected.\n",conn->fd());
    //Socket的析构函数会关闭fd,无需close(conn->fd());
    conns_.erase(conn->fd());
}

void TcpServer::errorconnection(spConnection conn)
{
    if(errorconnectioncb_)errorconnectioncb_(conn);

    //printf("client(eventfd=%d)error.\n",conn->fd());
    conns_.erase(conn->fd());
}

void TcpServer::onmessage(spConnection conn,std::string& message)
{
    if(onmessagecb_)onmessagecb_(conn,message);
}

void TcpServer::sendcomplete(spConnection conn)
{
    //printf("send complete.\n");
    //根据需求，可增加代码
    if(sendcompletecb_)sendcompletecb_(conn);
}      

void TcpServer::epolltimeout(EventLoop* loop)
{
    //printf("epoll_wait() timeout.\n");
    //根据需求，可增加代码
    if(timeoutcb_)timeoutcb_(loop);
}    

void TcpServer::setnewconnectioncb(std::function<void(spConnection)>fn)
{
    newconnectioncb_ = fn;
}

void TcpServer::setcloseconnectioncb(std::function<void(spConnection)>fn)
{
    closeconnectioncb_ = fn;
}

void TcpServer::seterrorconnectioncb(std::function<void(spConnection)>fn)
{
    errorconnectioncb_ = fn;
}

void TcpServer::setonmessagecb(std::function<void(spConnection,std::string&)>fn)
{
    onmessagecb_ = fn;
}

void TcpServer::setsendcompletecb(std::function<void(spConnection)>fn)
{
    sendcompletecb_ = fn;
}

void TcpServer::settimeoutcb(std::function<void(EventLoop*)>fn)
{
    timeoutcb_ = fn;
}
