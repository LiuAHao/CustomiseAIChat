#include"Socket.h"

//socket类
// class Socket {
// private:
//     const int fd_;  //socket类的文件标识符
    
// public:
//     Socket(int fd); //传入fd
//     ~Socket();

//     int fd() const;
//     void setreuseaddr(bool on); //设置SO_REUSEADDR选项，true为打开，false为关闭
//     void setreuseport(bool on); //设置SO_REUSEPORT选项，true为打开，false为关闭
//     void settcpnodelay(bool on); //设置TCP_NODELAY选项，true为打开，false为关闭
//     void setkeepalive(bool on); //设置SO_KEEPALIVE选项，true为打开，false为关闭
    
//     void bind(const InetAddress &servaddr);
//     void listen(int nn = 128);
//     int accept(InetAddress &clientaddr);
    
// };

int createnonblocking()
{
    //创建服务端监听套接字
    int listenfd = socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK,IPPROTO_TCP);   //SOCK_NONBLOCK直接创建非阻塞
    if(listenfd < 0){
        perror("socket() failed.\n");exit(-1);
    }
    return listenfd;
}

Socket::Socket(int fd):fd_(fd)
{
}

Socket::~Socket()
{
    ::close(fd_);
}

int Socket::fd() const
{
    return fd_;
}

void Socket::setreuseaddr(bool on)
{   
    int optval = on?1:0;
    ::setsockopt(fd_,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval)); 
}

void Socket::setreuseport(bool on)
{
    int optval = on?1:0;
    ::setsockopt(fd_,SOL_SOCKET,TCP_NODELAY,&optval,sizeof(optval));
}

void Socket::settcpnodelay(bool on)
{
    int optval = on?1:0;
    ::setsockopt(fd_,SOL_SOCKET,SO_REUSEPORT,&optval,sizeof(optval));
}

void Socket::setkeepalive(bool on)
{
    int optval = on?1:0;
    ::setsockopt(fd_,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof(optval));
}
    
void Socket::bind(const InetAddress &servaddr)
{
    if(::bind(fd_,servaddr.addr(),sizeof(sockaddr))< 0)
    {   
        perror("bind error.\n");close(fd_);exit(-1);
    }
}

void Socket::listen(int nn)
{
    if(::listen(fd_,nn) != 0)
    {
        perror("listen error.\n");close(fd_);exit(-1);
    }
}

int Socket::accept(InetAddress &clientaddr)
{
    struct sockaddr_in peeraddr;
    socklen_t len = sizeof(peeraddr);
    int clientfd = accept4(fd_,(struct sockaddr*)&peeraddr,&len,SOCK_NONBLOCK);  //直接设置非阻塞

    clientaddr.setaddr(peeraddr);
    return clientfd;
}