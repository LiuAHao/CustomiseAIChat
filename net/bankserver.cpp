/*
    网上银行（BankServer）服务器
    *用xml格式处理业务
    *增加登录，查询，注销，心跳业务
*/
#include<signal.h>
#include"BankServer.h"

BankServer* bankserver;      //设置为全局变量，让信号函数可以访问

//2和15信号的处理函数
void Stop(int sig)
{
    printf("sig=%d\n",sig);
    //printf("echoserver已经停止\n");
    bankserver->Stop();
    //调用EchoServer::Stop()函数
    delete bankserver;
    //printf("echoserver delete\n");
    exit(0);
}

int main(int argc,char *argv[]){
    if(argc != 3){
        printf("usage:./bankserver ip port\n");
        printf("example:./bankserver 192.168.232.129 5085\n\n");
        return -1;
    }

    signal(SIGTERM,Stop);       //15信号触发
    signal(SIGINT,Stop);        //2信号触发

    bankserver = new BankServer(argv[1],atoi(argv[2]),10,5);
    bankserver->Start();  //运行事件循环
   
    return 0;
}