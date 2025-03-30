/*
08版优化epoll模型
    修改各个函数，使用c++的绑定器和包装器实现函数回调

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

    Epoll ep;
    Channel *servchannel = new Channel(&ep,servsock.fd());
    servchannel -> setreadcallback(std::bind(&Channel::newconnection,servchannel,&servsock));
    servchannel -> enablereading();
    
    while(true)
    {
        std::vector<Channel*> channels;
        channels = ep.loop();    //等待监视的fd有事件发生
        
        for(auto &ch:channels)
        {
            ch->handleevent(); 
        }
    }
    return 0;
}