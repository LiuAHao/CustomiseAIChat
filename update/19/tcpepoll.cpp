/*

19版
   封装输入输出缓冲区Buffer
   在Connection类中，创建inputbuffer与outputbuffer缓冲区

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