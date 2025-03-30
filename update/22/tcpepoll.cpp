/*

22版
    优化回调函数，实现传输完毕或 错误时通知TcpServer
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