#include"Epoll.h"

// class Epoll{
//     private:
//         static const int MaxEvents = 100;   //epoll_wait()返回事件数组大小
//         int epollfd_ = -1;                  
//         epoll_event events_[MaxEvents];     //存放poll_wait()返回事件的数组
    
//     public:
//         Epoll();
//         ~Epoll();
//         void addfd(int fd,uint32_t op);     //把fd和需要监视的事件添加到红黑树中
//         std::vector<epoll_event> loop(int timeout = -1);    //运行epoll_wait(),等待事件的发生，已发生的事件用数组容器vector返回
    
//     };

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
/*
void Epoll::addfd(int fd,uint32_t op)
{
    epoll_event ev; //事件数据结构
    ev.data.fd = fd;
    ev.events = op;

    if(epoll_ctl(epollfd_,EPOLL_CTL_ADD,fd,&ev) == -1)    //把fd和它的事件加入epollfd队列
    {
        printf("epoll_ctl() failed(%d).\n",errno);
        exit(-1);
    }
}
*/

void Epoll::updatechannel(Channel *ch)
{
    epoll_event ev;
    ev.data.ptr = ch;
    ev.events = ch->events();

    if(ch->inpoll())    //已经在树上
    {
        if(epoll_ctl(epollfd_,EPOLL_CTL_MOD,ch->fd(),&ev) == -1)
        {
            perror("epoll_ctl() failed.\n");
        }
    }
    else                //未在树上
    {
        if(epoll_ctl(epollfd_,EPOLL_CTL_ADD,ch->fd(),&ev) == -1)
        {
            perror("epoll_ctl() failed.\n");
        }
        ch -> setinepoll();
    }
}
/*
std::vector<epoll_event> Epoll::loop(int timeout)
{
    std::vector<epoll_event> evs;

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
        return evs;
    }
    //如果返回infds > 0,代表事件发生fd的数量
    for(int ii = 0; ii < infds; ii++)   //遍历epoll
    {
        evs.push_back(events_[ii]); //将发生的事件加入到evs容器中
    }
    return evs;
}
*/

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

