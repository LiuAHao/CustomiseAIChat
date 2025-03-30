/*
38版
    基于Reactor服务器设计开发业务
    *服务器/客户端模式 
        *客户端：手机App，网站Web，前置服务器支付接口 

*/
#include<signal.h>
#include"EchoServer.h"

EchoServer* echoserver;      //设置为全局变量，让信号函数可以访问

//2和15信号的处理函数
void Stop(int sig)
{
    printf("sig=%d\n",sig);
    //printf("echoserver已经停止\n");
    echoserver->Stop();
    //调用EchoServer::Stop()函数
    delete echoserver;
    //printf("echoserver delete\n");
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

    echoserver = new EchoServer(argv[1],atoi(argv[2]),10,5);
    echoserver->Start();  //运行事件循环
   
    return 0;
}