#pragma once

#include<stdlib.h>
#include<stdio.h>
#include<errno.h>
#include<strings.h>
#include<string.h>
#include<sys/epoll.h>
#include<vector>
#include<unistd.h>
#include"Channel.h"

class Channel;              
/*
头文件循环依赖
如果 Epoll.h 和 Channel.h 互相包含（例如 Channel.h 中也包含 Epoll.h）,
会导致 循环依赖。即使有 #pragma once，
编译器在解析代码时可能会跳过某些类的定义，导致编译错误。

当 Epoll.h 中的代码（如成员函数 updatechannel(Channel *ch)）需要用到 Channel 类时，
如果 Channel.h 的头文件尚未被完整解析，
编译器无法识别 Channel 类型。此时，前向声明 class Channel; 
会告诉编译器：“Channel 是一个类，稍后会有定义”，从而允许编译通过。
*/

class Epoll{
private:
    static const int MaxEvents = 100;   //epoll_wait()返回事件数组大小
    int epollfd_ = -1;                  
    epoll_event events_[MaxEvents];     //存放poll_wait()返回事件的数组

public:
    Epoll();
    ~Epoll();
    //void addfd(int fd,uint32_t op);     //把fd和需要监视的事件添加到红黑树中
    std::vector<Channel*> loop(int timeout = -1);    //运行epoll_wait(),等待事件的发生，已发生的事件用数组容器vector返回
    void updatechannel(Channel *ch);       //把channel加入/更新到红黑树当中，channel中有fd，也有需要监视的事件
};