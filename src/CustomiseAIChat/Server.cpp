#include<signal.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include"AIServer.h"  // 修改为AIServer.h

AIServer* aiserver;  // 修改变量名为aiserver
pid_t python_pid = -1;  // 添加Python进程ID变量

void Stop(int sig){
    printf("sig=%d\n",sig);
    aiserver->Stop();
    delete aiserver;
    
    // 停止Python服务器进程
    if (python_pid > 0) {
        printf("正在停止Python服务器...\n");
        kill(python_pid, SIGTERM);
    }
    
    exit(0);
}

// 启动Python服务器的函数
void StartPythonServer() {
    python_pid = fork();
    if (python_pid == 0) {
        // 子进程执行Python脚本
        printf("正在启动Python AI服务器...\n");
        execlp("python3", "python3", "../python/ai_server.py", NULL);
        // 如果execlp执行失败
        perror("启动Python服务器失败");
        exit(1);
    } else if (python_pid < 0) {
        // fork失败
        perror("创建Python服务器进程失败");
    } else {
        // 父进程
        printf("Python服务器已启动，PID: %d\n", python_pid);
    }
}

int main(int argc,char *argv[]){
    if(argc != 3){
        printf("usage:./Server ip port\n");
        printf("example:./Server 192.168.232.129 5085\n\n");
        return -1;
    }

    signal(SIGTERM,Stop);
    signal(SIGINT,Stop);

    // 启动Python服务器
    StartPythonServer();
    
    // 启动C++服务器
    aiserver = new AIServer(argv[1],atoi(argv[2]),10,5);
    aiserver->Start();
   
    return 0;
}