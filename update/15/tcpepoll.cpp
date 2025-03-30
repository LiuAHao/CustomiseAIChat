/*

15版优化epoll模型
    优化回调函数中的Acceptor

*/

#include"TcpServer.h"

int main(int argc,char *argv[]){
    if(argc != 3){
        printf("usage:./tcpepoll ip port\n");
        printf("example:./tcpepoll 192.168.232.129 5085\n\n");
        return -1;
    }

    TcpServer tcpserver(argv[1],atoi(argv[2]));
    tcpserver.start();  //运行事件循环
   
    return 0;
}