/*
28版
   在多线程中如何管理资源
   *资源在多线程使用，资源是否有效，使用完如何析构
   *使用C++11智能指针管理资源，#include<memory> shared_ptr<>
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