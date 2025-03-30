/*
32版
    异步唤醒事件循环
    *为了防止工作线程和IO线程同时操作发送缓冲区，将工作线程的发送操作转移到IO线程中去，为每个IO线程的EventLoop增加一个任务队列
    *当工作线程将发送任务加到到IO线程工作队列时，唤醒IO线程（使用eventfd）
}
*/
#include"EchoServer.h"

int main(int argc,char *argv[]){
    if(argc != 3){
        printf("usage:./tcpepoll ip port\n");
        printf("example:./tcpepoll 192.168.232.129 5085\n\n");
        return -1;
    }

    EchoServer echoserver(argv[1],atoi(argv[2]),3,2);
    echoserver.Start();  //运行事件循环
   
    return 0;
}