/*
30版
    使用C++11智能指针管理资源，#include<memory> 
    *将部分裸指针替换为unique_ptr自动管理内存
    *部分使用堆内存的小型对象改用栈内存
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