/*
26版
    实现多线程的主从Reactor模型
    *单线程无法发挥多核CPU的性能
    *运行多个事件循环，主事件运行在主线程中，从事件运行在线程池中
    *主线程负责创建客户端连接，然后把conn分配给线程池
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