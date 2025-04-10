#include<signal.h>
#include"AIServer.h"  // 修改为AIServer.h

AIServer* aiserver;  // 修改变量名为aiserver

void Stop(int sig)
{
    printf("sig=%d\n",sig);
    aiserver->Stop();
    delete aiserver;
    exit(0);
}

int main(int argc,char *argv[]){
    if(argc != 3){
        printf("usage:./Server ip port\n");
        printf("example:./Server 192.168.232.129 5085\n\n");
        return -1;
    }

    signal(SIGTERM,Stop);
    signal(SIGINT,Stop);

    aiserver = new AIServer(argv[1],atoi(argv[2]),10,5);
    aiserver->Start();
   
    return 0;
}