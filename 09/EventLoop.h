#pragma once

#include"Epoll.h"

//事件循环类
class EventLoop
{
private:
    Epoll *ep_;          //每个时间循环只有一个epoll

public:
    EventLoop();    
    ~EventLoop();

    void run();         //运行事件循环
    Epoll* ep();
};