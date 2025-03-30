/*
27版
   增加工作线程
   *一个Reactor负责多个Connection，Connection负责IO和计算，IO不会阻塞事件，计算会阻塞事件循环，其他事件无法执行
   *增加工作线程用来解决计算阻塞问题，计算工作交给工作线程
*/
#include"EchoServer.h"

int main(int argc,char *argv[]){
    if(argc != 3){
        printf("usage:./tcpepoll ip port\n");
        printf("example:./tcpepoll 192.168.232.129 5085\n\n");
        return -1;
    }

    EchoServer echoserver(argv[1],atoi(argv[2]));
    echoserver.Start();  //运行事件循环
   
    return 0;
}