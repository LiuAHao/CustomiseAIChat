/*
09版优化epoll模型
    增加EventLoop类，封装epoll和事件循环

*/

#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<sys/fcntl.h>
#include<sys/epoll.h>
#include<netinet/tcp.h>
#include"InetAddress.h"
#include"Socket.h"
#include"Epoll.h"
#include"EventLoop.h"


int main(int argc,char *argv[]){
    if(argc != 3){
        printf("usage:./tcpepoll ip port\n");
        printf("example:./tcpepoll 192.168.232.129 5085\n\n");
        return -1;
    }
    
    Socket servsock(createnonblocking());
    InetAddress servaddr(argv[1],atoi(argv[2]));
    servsock.setreuseaddr(true); 
    servsock.settcpnodelay(true);
    servsock.setreuseport(true);
    servsock.setkeepalive(true);
   
    servsock.bind(servaddr);
    servsock.listen();

    EventLoop loop;
    Channel *servchannel = new Channel(loop.ep(),servsock.fd());
    servchannel -> setreadcallback(std::bind(&Channel::newconnection,servchannel,&servsock));
    servchannel -> enablereading();

    loop.run();
   
    return 0;
}