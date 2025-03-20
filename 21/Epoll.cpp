#include"Epoll.h"

Epoll::Epoll()
{
    if((epollfd_ = epoll_create(1)) == -1){
        printf("epollfd_create() failed(%d).\n",errno);
        exit(-1);
    }
}

Epoll::~Epoll()
{
    close(epollfd_);
}

int Epoll::fd()
{
    return epollfd_;
}


std::vector<Channel*> Epoll::loop(int timeout)
{
    std::vector<Channel*> channels;

    bzero(events_,sizeof(events_));
    int infds = epoll_wait(epollfd_,events_,MaxEvents,-1);  //等待监视的fd有事件发生，发生的事件会加入到events中
    //返回失败 
    if(infds < 0)//如果infds > 0,表示fd发生事件的数量
    {
        perror("epoll_wait failed.\n");
        exit(-1);
    }
    //超时
    if(infds == 0)
    {
        printf("epoll_wait() timeout.\n");
        return channels;
    }
    //如果返回infds > 0,代表事件发生fd的数量
    for(int ii = 0; ii < infds; ii++)   //遍历epoll
    {
        Channel *ch = (Channel*)events_[ii].data.ptr;
        ch -> setrevents(events_[ii].events);   //设置channel的已发生事件
        channels.push_back(ch); //将发生的事件加入到evs容器中
    }
    return channels;
}

