#include"EventLoop.h"

int createtimefd(int sec = 30)
{
    int tfd = timerfd_create(CLOCK_MONOTONIC,TFD_CLOEXEC|TFD_NONBLOCK);     //创建timerfd
    struct itimerspec timeout;                      //定时时间数据结构
    memset(&timeout,0,sizeof(struct itimerspec));   //初始化时间结构体
    timeout.it_value.tv_sec = sec;    //等待的时间(5秒)
    timeout.it_value.tv_nsec = 0;   //纳秒
    timerfd_settime(tfd,0,&timeout,0);
    return tfd;
}

EventLoop::EventLoop(bool mainloop,int timevl,int timeout):mainloop_(mainloop),timevl_(timevl),timeout_(timeout)
        ,ep_(new Epoll),wakeupfd_(eventfd(0,EFD_NONBLOCK)),wakeupchannel_(new Channel(this,wakeupfd_))
        ,timerfd_(createtimefd()),timerchannel_(new Channel(this,timerfd_)),stop_(false)
{
    wakeupchannel_->setreadcallback(std::bind(&EventLoop::handlewakeup,this));  //将handlewakeup()函数设置到事件循环的读回调函数中
    wakeupchannel_->enablereading();    //触发设置的读回调->handlewakeup()
    timerchannel_->setreadcallback(std::bind(&EventLoop::handletimer,this)); //handletimer()函数设置到事件循环的读回调函数中,timefd_触发就回调
    timerchannel_->enablereading();     //触发设置的读回调->handletimer()
} 

EventLoop::~EventLoop()
{
}

void EventLoop::run()
{
    //printf("EventLoop::run() thread is %d.\n",(int)syscall(SYS_gettid));
    threadid_ = syscall(SYS_gettid);    //获取事件循环所在线程的id
    while(stop_ == false)
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

void EventLoop::stop()
{
    stop_ = true;       //设置时并不会直接退出事件循环需要等到事件发生才会退出，需要唤醒事件循环
    wakeup();
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

bool EventLoop::isinloopthread()
{
    return threadid_ == syscall(SYS_gettid);    //如果在工作线程中两者就不相等
}

void EventLoop::queueinloop(std::function<void()> fn)
{
    {
        std::lock_guard<std::mutex> gd(mutex_);
        taskqueue_.push(fn);
    }
    
    //唤醒事件循环
    wakeup();
}

void EventLoop::wakeup()
{
    uint64_t val = 1;
    write(wakeupfd_,&val,sizeof(val));  //唤醒线程，写什么不重要
}

void EventLoop::handlewakeup()
{
    //printf("handlewakeup() thread id is %d\n",(int)syscall(SYS_gettid));

    uint64_t val;
    read(wakeupfd_,&val,sizeof(val));   //读取数据，数据不重要，read代表唤醒成功开始执行  

    std::function<void()> fn;
    
    std::lock_guard<std::mutex> gd(mutex_);
    
    while(taskqueue_.size() > 0)
    {
        fn = std::move(taskqueue_.front());     //使用move避免重复构造
        taskqueue_.pop();
        //printf("执行任务队列中的任务\n");  // 添加日志
        fn();
    }       
}

void EventLoop::handletimer()
{
    //重新开始计时
    struct itimerspec timeout;                      //定时时间数据结构
    memset(&timeout,0,sizeof(struct itimerspec));   //初始化时间结构体
    timeout.it_value.tv_sec = timevl_;    //等待的时间(5秒)
    timeout.it_value.tv_nsec = 0;   //纳秒
    timerfd_settime(timerfd_,0,&timeout,0);
    if(mainloop_)
    {
        
    }
    else
    {
        time_t now = time(0);
        //printf("EventLoop::handletimer() threadid is %d fd:",(int)syscall(SYS_gettid));
        // for(auto conn:conns_)
        // {
        //     printf("%d ",conn.first);
        //     if(conn.second->timeout(now,10))
        //     {
                
        //         timercallback_(conn.first);       //从TcpServer中删去Connection
        //         conns_.erase(conn.first);       //超时则删去Connection  //Connection的释放出现段错误
        //     }
        // }
        //上面的代码因为map的erase函数会段错误错误，怀疑是for迭代使得conn指针指向空内存
        std::map<int,spConnection>::iterator it = conns_.begin();  
        while(it != conns_.end())  
        {  
            if(it->second->timeout(now,timeout_))  
            {  
                timercallback_(it->first); 
                {
                    std::lock_guard<std::mutex> gd(mmutex_);    //加锁
                    it = conns_.erase(it);  
                }
            }  
            else  
                it++;  
        }  
    }
}

void EventLoop::newconnection(spConnection conn)
{
    {
        std::lock_guard<std::mutex> gd(mmutex_);    //加锁
        conns_[conn->fd()] = conn;
    }
}

void EventLoop::settimercallback(std::function<void(int)> fn)
{
    timercallback_ = fn;
} 
