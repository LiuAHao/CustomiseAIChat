/*
33版
    性能优化
    *长期没有通讯的tcp连接为空闲的Connection
    *需要定时清理空闲的Connection
    *定时器用来执行定时任务，alarm()设置定时器触发SIGALRM信号
    *signal(SIGALRM,std::function 定时执行函数体); alarm(int 计时秒数);在alarm函数执行后过的秒数执行函数体
    *定时器和信号会被内核统一成fd，由epoll统一管理
    *增加时间戳类Timestamp
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