/*
34版
    定时清理空闲的Connection
    流程：
    *1.事件循环中加入map<int,spConnection> conn_容器，存放运行在事件循环上的Connection对象
    *2.如果闹钟时间到了，遍历Connection对象，判断每一个Connection对象是否超时
    *3.如果超时了，就从conn_中删除Connection对象
    *4.还需要从TcpServer.conn_中删除Connection对象
    *5.TcpServer和EventLoop的对conns_的map操作需要加锁，防止主事件循环和从事件循环冲突
    *6.闹钟时间参数化
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