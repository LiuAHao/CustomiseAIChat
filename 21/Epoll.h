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

class Epoll{
private:
    static const int MaxEvents = 100;   //epoll_wait()返回事件数组大小
    int epollfd_ = -1;                  
    epoll_event events_[MaxEvents];     //存放poll_wait()返回事件的数组

public:
    Epoll();
    ~Epoll();
    int fd();
    //void addfd(int fd,uint32_t op);     //把fd和需要监视的事件添加到红黑树中
    std::vector<Channel*> loop(int timeout = -1);    //运行epoll_wait(),等待事件的发生，已发生的事件用数组容器vector返回
};