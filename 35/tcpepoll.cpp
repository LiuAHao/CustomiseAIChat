/*
35版
    让多线程服务体面的退出
    *删除日志输出
    *用ctrl+C太暴力,体面退出保留资源
    *利用信号处理退出
        1.设置2和15信号
        2.在信号处理函数中停止主从事件循环和工作线程
        3.服务程序停止服务
    *增加线程池停止线程函数
}
*/
#include<signal.h>
#include"EchoServer.h"

EchoServer* echoserver;      //设置为全局变量，让信号函数可以访问

//2和15信号的处理函数
void Stop(int sig)
{
    printf("sig=%d\n",sig);
    printf("echoserver已经停止\n");
    echoserver->Stop();
    //调用EchoServer::Stop()函数
    delete echoserver;
    printf("echoserver delete\n");
    exit(0);
}

int main(int argc,char *argv[]){
    if(argc != 3){
        printf("usage:./tcpepoll ip port\n");
        printf("example:./tcpepoll 192.168.232.129 5085\n\n");
        return -1;
    }

    signal(SIGTERM,Stop);       //15信号触发
    signal(SIGINT,Stop);        //2信号触发

    echoserver = new EchoServer(argv[1],atoi(argv[2]),3,2);
    echoserver->Start();  //运行事件循环
   
    return 0;
}