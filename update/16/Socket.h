#pragma once
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/tcp.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>
#include"InetAddress.h"

int createnonblocking();    //对于监听的fd，我们需要返回一个非阻塞的fd用于时刻监听连接

//socket类
class Socket {
private:
    const int fd_;  //socket类的文件标识符
    std::string ip_;    
    uint16_t port_;

public:
    Socket(int fd); //传入fd
    ~Socket();

    int fd() const;
    std::string ip();
    uint16_t port();
    void setreuseaddr(bool on); //设置SO_REUSEADDR选项，true为打开，false为关闭
    void setreuseport(bool on); //设置SO_REUSEPORT选项，true为打开，false为关闭
    void settcpnodelay(bool on); //设置TCP_NODELAY选项，true为打开，false为关闭
    void setkeepalive(bool on); //设置SO_KEEPALIVE选项，true为打开，false为关闭

    void bind(const InetAddress &servaddr);
    void listen(int nn = 128);
    int accept(InetAddress &clientaddr);

};