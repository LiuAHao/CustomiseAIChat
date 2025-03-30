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

void EventLoop::updatechannel(Channel *ch)
{
    epoll_event ev;
    ev.data.ptr = ch;
    ev.events = ch->events();
    if(ch->inpoll())    //已经在树上
    {
        if(epoll_ctl(ep_->fd(),EPOLL_CTL_MOD,ch->fd(),&ev) == -1)
        {
            perror("epoll_ctl() failed.\n");
        }
    }
    else                //未在树上
    {
        if(epoll_ctl(ep_->fd(),EPOLL_CTL_ADD,ch->fd(),&ev) == -1)
        {
            perror("epoll_ctl() failed.\n");
        }
        ch -> setinepoll();
    }
}