/*
37版
    测试性能，百万秒吞吐量
    *终端显示需要时间，不适合性能测试
    *增加tmp1.sh执行脚本（&代表并行执行，无需等待）实现多客户端传输
    *最终效果十台客户端十万传输，共百万吞吐50秒完成，约两万每秒吞吐量
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

    echoserver = new EchoServer(argv[1],atoi(argv[2]),10,0);
    echoserver->Start();  //运行事件循环
   
    return 0;
}