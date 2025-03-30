#include"Acceptor.h"

// class Acceptor
// {
// private:
//     EventLoop* loop_;           //对应事件循环
//     Socket* servsock_;          //服务端用于监听的socket
//     Channel* acceptchannel_;    //对应的channel
// public:
//     Acceptor(EventLoop* loop_,const std::string &ip,uint16_t port);
//     ~Acceptor();

// };

Acceptor::Acceptor(EventLoop* loop,const std::string &ip,uint16_t port):loop_(loop)
{
    servsock_ = new Socket(createnonblocking());
    InetAddress servaddr(ip,port);
    servsock_->setreuseaddr(true); 
    servsock_->settcpnodelay(true);
    servsock_->setreuseport(true);
    servsock_->setkeepalive(true);
   
    servsock_->bind(servaddr);
    servsock_->listen();

    acceptchannel_ = new Channel(loop_,servsock_->fd());
    acceptchannel_ -> setreadcallback(std::bind(&Channel::newconnection,acceptchannel_,servsock_));
    acceptchannel_ -> enablereading();
}

Acceptor::~Acceptor()
{
    delete servsock_;
    delete acceptchannel_;
}