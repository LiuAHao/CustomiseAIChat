/*
29版
    使用C++11智能指针管理资源，#include<memory> shared_ptr<>
    *利用继承public std::enable_shared_from_this<Connection> 来使得shared_from_this()函数指向自己的智能指针
    *若连接断开，应该取消关注Channel所有的事件
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