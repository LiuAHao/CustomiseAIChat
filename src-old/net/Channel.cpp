#include"Channel.h"

Channel::Channel(EventLoop* loop,int fd):loop_(loop),fd_(fd)
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
    events_ |= EPOLLIN;
    loop_->updatechannel(this);
}

void Channel::disablereading()
{
    events_ &= ~EPOLLIN;
    loop_->updatechannel(this);
}

void Channel::enablewriting()   //成功进入
{
    events_ |= EPOLLOUT;
    loop_->updatechannel(this);
}

void Channel::disablewriting()
{
    events_ &= ~EPOLLOUT;
    loop_->updatechannel(this);
}

void Channel::disableall()
{
    events_ = 0;
    loop_->updatechannel(this);
}

void Channel::remove()
{
    disableall();
    loop_->removechannel(this);
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

void Channel::handleevent()
{
    if(revents_ & EPOLLRDHUP) //对方已关闭
    {            
        //printf("EPOLLRDHUP\n");
        closecallback_();
    }                 
    else if(revents_ & (EPOLLIN|EPOLLPRI))  //接收缓冲区有数据可读,IN为读
    {      
        //printf("EPOLLIN|EPOLLPRI\n");
        readcallback_();
    }
    else if(revents_ & EPOLLOUT)  //有数据需要写，OUT为写
    {      
        //printf("EPOLLOUT\n");
        writecallback_();       //回调Connection::writecallback()函数
    }              
    else    //其他情况都视为错误
    {             
        //printf("OTHERS\n");                            
        errorcallback_();
    }                            
}


void Channel::setreadcallback(std::function<void()>fn)
{
    readcallback_ = fn;
}

void Channel::setclosecallback(std::function<void()>fn)
{
    closecallback_ = fn;
}

void Channel::seterrorcallback(std::function<void()>fn)
{   
    errorcallback_ = fn;
}

void Channel::setwritecallback(std::function<void()>fn)
{
    writecallback_ = fn;
}

