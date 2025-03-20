#include"Channel.h"


// class Channel
// {
// private:
//     int fd_;                    //Channel拥有的id，一一对应
//     Epoll* ep_ = nullptr;        //Channel对应的红黑树，一一对应
//     bool inepoll_ = false;      //标识Channel是否已经添加到epoll中，未添加：调用epoll_ctl时用EPOLL_CTL_ADD，反之用EPOLL_CTL_MOD
//     uint32_t events_ = 0;       //fd_需要监视的事件
//     uint32_t revents_ = 0;      //fd_已经发生的事件
// public:
//     Channel(Epoll* ep,int fd);
//     ~Channel();

//     int fd();
//     void usset();                   //采用边缘触发
//     void enablereading();
//     void setinepoll();              //epoll_wait()监听fd_的读事件
//     void setrevents(uint32_t ev);   //设置revents_成员为ev
//     bool inpoll();
//     uint32_t events();
//     uint32_t revents();
    
// };
Channel::Channel(Epoll* ep,int fd):ep_(ep),fd_(fd)
{
}

Channel::~Channel()
{
}

int Channel::fd()
{
    return fd_;
}

void Channel::usset()
{   
    events_ = events_|EPOLLET;  //边缘触发
}                 

void Channel::enablereading()
{
    events_ = events_|EPOLLIN;
    ep_ -> updatechannel(this);
}

void Channel::setinepoll()
{
    inepoll_ = true;
}          
void Channel::setrevents(uint32_t ev)
{
    revents_ = ev;
}
bool Channel::inpoll()
{
    return inepoll_;
}
uint32_t Channel::events()
{
    return events_;
}
uint32_t Channel::revents()
{
    return revents_;
}