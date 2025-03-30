#include"EventLoop.h"

EventLoop::EventLoop():ep_(new Epoll)
{

} 

EventLoop::~EventLoop()
{
    delete ep_;
}

void EventLoop::run()
{
    while(true)
    {
        std::vector<Channel*> channels = ep_->loop();//等待监视的fd有事件发生

        for(auto &ch:channels)
        {
            ch->handleevent(); 
        }
    }
}

Epoll* EventLoop::ep()
{
    return ep_;
}
