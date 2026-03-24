#pragma once

#include<memory>
#include<functional>
#include<unistd.h>
#include<sys/syscall.h>
#include<queue>
#include<mutex>
#include<map>
#include<atomic>
#include<sys/eventfd.h>
#include<sys/timerfd.h>
#include"Epoll.h"
#include"Connection.h"

class Epoll;
class Channel;
class Connection;
using spConnection = std::shared_ptr<Connection>;
//事件循环类
class EventLoop
{
private:
    int timevl_;                //闹钟时间间隔（秒）
    int timeout_;               //Connection对象超时时间
    std::unique_ptr<Epoll> ep_;          //每个时间循环只有一个epoll
    std::function<void(EventLoop*)> epolltimeoutcallback_;  //epoll_wait()超时的回调函数
    pid_t threadid_;
    std::queue<std::function<void()>> taskqueue_;   //事件循环线程被eventfd唤醒后执行的任务队列
    std::mutex mutex_;                          //任务队列同步互斥锁
    int wakeupfd_;
    std::unique_ptr<Channel> wakeupchannel_;
    int timerfd_;                           //定时器的fd
    std::unique_ptr<Channel> timerchannel_; //定时器的channel
    bool mainloop_;                         //判断是否为主事件循环，主事件循环不用定时管理Connection
    std::mutex mmutex_;         //保护conns_的互斥锁

    std::map<int,spConnection> conns_;       //存放运行事件循环中的Connection对象

    std::function<void(int)> timercallback_;    //超时删除回调函数
    std::function<void(int)> closecallback_;    //关闭连接回调函数

    std::atomic_bool stop_;                 //停止运行标志位，退出死循环
public:
    EventLoop(bool mainloop,int timevl_ = 30,int timeout_ = 80);    
    ~EventLoop();

    void run();         //运行事件循环
    void stop();
    
    void updatechannel(Channel* ch);       //把channel加入/更新到红黑树当中，channel中有fd，也有需要监视的事件
    void removechannel(Channel* ch);        //从红黑树epoll中删除channel
    void setepolltimeoutcallback(std::function<void(EventLoop*)> fn);   //设置epoll_wait()超时的回调函数

    bool isinloopthread();          //判断当前线程是否为事件循环线程

    void queueinloop(std::function<void()> fn); //任务加到任务队列
    void wakeup();                  //唤醒事件循环
    void handlewakeup();            //事件循环被唤醒后执行的函数   

    void handletimer();             //定时器的回调函数  

    void newconnection(spConnection conn);  //把Connection存放到conns_中
    void settimercallback(std::function<void(int)> fn);

};