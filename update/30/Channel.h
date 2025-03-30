#pragma once

#include<sys/epoll.h>
#include<functional>
#include<memory>
#include"Epoll.h"
#include"InetAddress.h"
#include"Socket.h"
#include"EventLoop.h"

class Epoll;
class EventLoop;

class Channel                   //Channel类是Acceptor类和Connection类的下层类
{
private:
    int fd_;                    //Channel拥有的id，一一对应
    const std::unique_ptr<EventLoop>& loop_;        //Channel对应的红黑树，一一对应
    bool inepoll_ = false;      //标识Channel是否已经添加到epoll中，未添加：调用epoll_ctl时用EPOLL_CTL_ADD，反之用EPOLL_CTL_MOD
    uint32_t events_ = 0;       //fd_需要监视的事件
    uint32_t revents_ = 0;      //fd_已经发生的事件
    std::function<void()> readcallback_;        //fd_读事件的回调函数
    std::function<void()> closecallback_;       //关闭连接事件的回调函数,回调Connection::closecallback();
    std::function<void()> errorcallback_;       //连接错误事件的回调函数,回调Connection::errorcallback();
    std::function<void()> writecallback_;       //写事件事件的回调函数,回调Connection::writecallback();


public:
    Channel(const std::unique_ptr<EventLoop>& loop,int fd);    
    ~Channel();

    int fd();
    void usset();                   //采用边缘触发
    void enablereading();           //epoll_wait()监视读事件，注册读事件
    void disablereading();          //取消读事件
    void enablewriting();           //注册写事件
    void disablewriting();          //取消写事件
    void disableall();              //取消关注全部事件
    void remove();                  //从时间循环中删除Channel
    void setinepoll();              //epoll_wait()监听fd_的读事件
    void setrevents(uint32_t ev);   //设置revents_成员为ev
    bool inpoll();
    uint32_t events();
    uint32_t revents();

    void handleevent();             //事件处理函数，epoll_wait()返回的时候，执行它

    //void onmessage();                                   //处理对端发送的消息
    void setreadcallback(std::function<void()>fn);      //设置fd_读事件的回调函数
    void setclosecallback(std::function<void()>fn);     //设置关闭连接事件的回调函数
    void seterrorcallback(std::function<void()>fn);     //设置连接错误事件的回调函数
    void setwritecallback(std::function<void()>fn);     //设置写事件的回调函数

};