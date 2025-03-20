#pragma once

#include"Epoll.h"
class Epoll;
class Channel;
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
    void updatechannel(Channel *ch);       //把channel加入/更新到红黑树当中，channel中有fd，也有需要监视的事件
};