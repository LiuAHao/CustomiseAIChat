/*
31版
    学习的知识点：
    使用eventfd实现事件通知：
    *通知线程的方法：条件变量，信号量，socket，管道，eventfd
    *事件循环阻塞在epoll_wait()函数，条件变量，信号量有自己的等待函数(等待阻塞重复)，不适合通知事件循环
    *socket，管道，eventfd都是fd，可加入epoll，通知事件循环往eventfd，管道中写入数据即可
    *eventfd是Linux 提供的一种轻量级事件通知机制，常用于线程或进程间的通信
        *eventfd的write()和read()用于操作一个64为无符号数的整数计数器uint64_t
        *int efd = eventfd(initval, flags);         //创建eventfd
            *initval: 初始计数器值（一般为 0）
            *flags: 控制 eventfd 行为的标志，常用值：
                EFD_CLOEXEC: 执行 exec 时关闭文件描述符
                EFD_NONBLOCK: 设置非阻塞模式，read读到0时返回-1
                EFD_SEMAPHORE: 信号量模式
        *write：触发事件
        *read：等待事件，当读取到的数为0时阻塞
        *默认模式	read 后计数器归零，适合一次性事件通知（例如“有数据待处理”）
        *信号量模式	read 后计数器减 1，适合资源计数（例如“剩余可处理的任务数”）
    *管道（Pipe）是一种 进程间通信（IPC） 机制，允许两个进程（通常是父子进程）通过内核缓冲区传递数据。管道是单向的，数据只能从一端写入，从另一端读取。
        *int pipe_fd[2];
        if (pipe(pipe_fd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }   //创建无名管道，pipe_fd[0]：管道的读端（用于读取数据），pipe_fd[1]：管道的写端（用于写入数据）
        *write：
            const char* data = "Hello from writer!";
            ssize_t bytes_written = write(pipe_fd[1], data, strlen(data));//将数据写入管道的写端（pipe_fd[1]）
            *如果管道有空间，数据被写入内核缓冲区;如果管道已满，write 会阻塞，直到有空间可用（默认阻塞模式）
        *read:
            char buffer[256];
            ssize_t bytes_read = read(pipe_fd[0], buffer, sizeof(buffer));//从管道的读端（pipe_fd[0]）读取数据
            *如果管道有数据，read 立即返回可用数据;如果管道为空，read 会阻塞，直到有数据写入（默认阻塞模式）

    *指针的正确使用：
        *生命周期难以确定，用shared_ptr
        *类自己的拥有资源使用unique_ptr，自动销毁
        *不属于自己但会使用的资源，用裸指针
    代码处理：
    异步唤醒事件循环
    *为了防止工作线程和IO线程同时操作发送缓冲区，将工作线程的发送操作转移到IO线程中去，为每个IO线程的EventLoop增加一个任务队列
    *当工作线程将发送任务加到到IO线程工作队列时，唤醒IO线程（使用eventfd）
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