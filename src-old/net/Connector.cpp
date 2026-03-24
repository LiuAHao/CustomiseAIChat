#include"Connector.h"

Connector::Connector(EventLoop* loop,const std::string &ip,uint16_t port)
            :loop_(loop),servaddr_(ip,port),sockfd_(-1),connected_(false)
{
}

Connector::~Connector()
{
    if(connectchannel_)
    {
        connectchannel_->remove();
    }
    if(!connected_ && sockfd_ >= 0)
    {
        ::close(sockfd_);
    }
}

void Connector::start()
{
    sockfd_ = createnonblocking();      //创建非阻塞socket

    int ret = ::connect(sockfd_,servaddr_.addr(),sizeof(sockaddr));
    int savedErrno = (ret == 0) ? 0 : errno;

    switch(savedErrno)
    {
        case 0:             //连接立即成功（本地连接可能出现）
        case EINPROGRESS:   //连接正在进行中（非阻塞connect的正常返回）
        case EINTR:         //被信号中断
        case EISCONN:       //已经连接
        {
            //创建Channel监视连接完成事件（EPOLLOUT表示可写，即连接完成）
            connectchannel_.reset(new Channel(loop_,sockfd_));
            connectchannel_->setwritecallback(std::bind(&Connector::handlewrite,this));
            connectchannel_->seterrorcallback(std::bind(&Connector::handleerror,this));
            connectchannel_->setclosecallback(std::bind(&Connector::handleerror,this));
            connectchannel_->setreadcallback(std::bind(&Connector::handleerror,this));
            connectchannel_->enablewriting();
            break;
        }
        default:
        {
            printf("Connector::start() connect(%s:%d) failed: %s\n",
                servaddr_.ip(),servaddr_.port(),strerror(savedErrno));
            ::close(sockfd_);
            sockfd_ = -1;
            if(errorconnectioncb_) errorconnectioncb_();
            break;
        }
    }
}

void Connector::handlewrite()
{
    //检查连接是否真正成功（通过SO_ERROR判断）
    int err = 0;
    socklen_t len = sizeof(err);
    ::getsockopt(sockfd_,SOL_SOCKET,SO_ERROR,&err,&len);

    if(err != 0)
    {
        printf("Connector::handlewrite() connect failed: %s\n",strerror(err));
        connectchannel_->remove();
        connectchannel_.reset();
        ::close(sockfd_);
        sockfd_ = -1;
        if(errorconnectioncb_) errorconnectioncb_();
        return;
    }

    //连接成功，先从epoll中移除Connector的Channel
    connected_ = true;
    connectchannel_->remove();
    connectchannel_.reset();

    //创建Socket对象并通过回调传递给TcpClient
    std::unique_ptr<Socket> clientsock(new Socket(sockfd_));
    clientsock->setipport(servaddr_.ip(),servaddr_.port());

    if(newconnectioncb_) newconnectioncb_(std::move(clientsock));
}

void Connector::handleerror()
{
    int err = 0;
    socklen_t len = sizeof(err);
    if(sockfd_ >= 0)
        ::getsockopt(sockfd_,SOL_SOCKET,SO_ERROR,&err,&len);

    printf("Connector::handleerror() error: %s\n",strerror(err));

    if(connectchannel_)
    {
        connectchannel_->remove();
        connectchannel_.reset();
    }
    if(sockfd_ >= 0)
    {
        ::close(sockfd_);
        sockfd_ = -1;
    }

    if(errorconnectioncb_) errorconnectioncb_();
}

void Connector::setnewconnectioncb(std::function<void(std::unique_ptr<Socket>)> fn)
{
    newconnectioncb_ = fn;
}

void Connector::seterrorconnectioncb(std::function<void()> fn)
{
    errorconnectioncb_ = fn;
}
