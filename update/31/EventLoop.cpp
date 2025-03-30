#include"EventLoop.h"

EventLoop::EventLoop():ep_(new Epoll)
{
} 

EventLoop::~EventLoop()
{
}



void EventLoop::run()
{
    //printf("EventLoop::run() thread is %d.\n",(int)syscall(SYS_gettid));

    while(true)
    {
        std::vector<Channel*> channels = ep_->loop(10*1000);//等待监视的fd有事件发生

        //如果Channels为空，表示超时，回调TcpServer::epolltimeout()
        if(channels.size() == 0){
            epolltimeoutcallback_(this);
        }
        else{
            for(auto &ch:channels)
            {
                ch->handleevent(); 
            }
        }
        
    }
}

void EventLoop::updatechannel(Channel *ch)
{
    ep_->updatechannel(ch);
}

void EventLoop::removechannel(Channel* ch)
{
    ep_ ->removechannel(ch);
}

void EventLoop::setepolltimeoutcallback(std::function<void(EventLoop*)> fn)
{
    epolltimeoutcallback_ = fn;
}
