#include"EventLoop.h"

EventLoop::EventLoop():ep_(new Epoll),wakeupfd_(eventfd(0,EFD_NONBLOCK)),wakeupchannel_(new Channel(this,wakeupfd_))
{
    wakeupchannel_->setreadcallback(std::bind(&EventLoop::handlewakeup,this));  //将handlewakeup()函数设置到事件循环的读回调函数中，
    wakeupchannel_->enablereading();    //触发设置的读回调->handlewakeup()
    
} 

EventLoop::~EventLoop()
{
}

void EventLoop::run()
{
    //printf("EventLoop::run() thread is %d.\n",(int)syscall(SYS_gettid));
    threadid_ = syscall(SYS_gettid);    //获取事件循环所在线程的id
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
    printf("handlewakeup() thread id is %d\n",(int)syscall(SYS_gettid));

    uint64_t val;
    read(wakeupfd_,&val,sizeof(val));   //读取数据，数据不重要，read代表唤醒成功开始执行  

    std::function<void()> fn;
    
    std::lock_guard<std::mutex> gd(mutex_);
    
    while(taskqueue_.size() > 0)
    {
        fn = std::move(taskqueue_.front());     //使用move避免重复构造
        taskqueue_.pop();
        printf("执行任务队列中的任务\n");  // 添加日志
        fn();
    }       
}
