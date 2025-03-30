/*

17版
    用map容器管理Connection生命周期
    继续优化从Channel到TcpServer的回调函数
    完成从Channel到Connection的回调函数
18版
    用map容器管理Connection生命周期
    继续优化从Channel到TcpServer的回调函数
    完成从Connection到TcpServer的回调函数
*/

#include"TcpServer.h"

int main(int argc,char *argv[]){
    if(argc != 3){
        printf("usage:./tcpepoll ip port\n");
        printf("example:./tcpepoll 192.168.232.129 5085\n\n");
        return -1;
    }

    TcpServer tcpserver(argv[1],atoi(argv[2]));
    tcpserver.start();  //运行事件循环
   
    return 0;
}