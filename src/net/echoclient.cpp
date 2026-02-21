//异步回显客户端，使用TcpClient进行非阻塞通信
//与EchoServer配合使用，演示完整的异步客户端使用方式
#include"TcpClient.h"
#include<signal.h>

TcpClient* client;
int msgcount = 0;       //已发送的消息计数

void Stop(int sig)
{
    printf("\n正在停止客户端...\n");
    client->stop();
}

int main(int argc,char* argv[])
{
    if(argc != 3)
    {
        printf("usage: ./echoclient ip port\n");
        printf("example: ./echoclient 192.168.232.129 5085\n\n");
        return -1;
    }

    signal(SIGTERM,Stop);
    signal(SIGINT,Stop);

    client = new TcpClient(argv[1],atoi(argv[2]));

    //设置连接成功回调：连接建立后发送第一条消息
    client->setconnectioncb([](spConnection conn){
        printf("连接成功(fd=%d, server=%s:%d)\n",conn->fd(),conn->ip().c_str(),conn->port());
        msgcount = 1;
        std::string msg = "async hello " + std::to_string(msgcount);
        conn->send(msg.data(),msg.size());
        printf("发送: %s\n",msg.c_str());
    });

    //设置收到消息回调：打印回复，继续发送下一条（ping-pong模式）
    client->setmessagecb([](spConnection conn,std::string& message){
        printf("接收: %s\n",message.c_str());
        msgcount++;
        if(msgcount <= 10)
        {
            std::string msg = "async hello " + std::to_string(msgcount);
            conn->send(msg.data(),msg.size());
            printf("发送: %s\n",msg.c_str());
        }
        else
        {
            printf("完成 %d 条消息的收发，退出。\n",msgcount - 1);
            client->stop();
        }
    });

    //设置连接断开回调
    client->setclosecb([](spConnection conn){
        printf("连接断开(fd=%d)\n",conn->fd());
    });

    //设置连接错误回调
    client->seterrorcb([](spConnection conn){
        printf("连接错误(fd=%d)\n",conn->fd());
    });

    //设置发送完毕回调
    client->setsendcompletecb([](spConnection conn){
        //printf("发送完毕(fd=%d)\n",conn->fd());
    });

    client->start();    //发起连接并启动事件循环（阻塞直到stop()被调用）

    delete client;
    printf("客户端已退出。\n");
    return 0;
}
