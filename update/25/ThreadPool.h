#pragma once


#include<iostream>
#include<sys/syscall.h>
#include<queue>
#include<vector>
#include<mutex>
#include<unistd.h>
#include<thread>
#include<condition_variable>
#include<functional>
#include<future>
#include<atomic>

class ThreadPool
{
private:
    std::vector<std::thread> threads_;              //线程池线程
    std::queue<std::function<void()>> taskqueue_;   //任务队列
    std::mutex mutex_;                              //任务队列互斥锁
    std::condition_variable condition_;             //任务队列同步条件变量
    std::atomic_bool stop_;                         //析构函数中，stop_为true代表全部线程已经退出 
public:
    ThreadPool(size_t threadnum);                   //启动threadnum个线程
    ~ThreadPool();                                  //停止线程
    void addtask(std::function<void()> task);       //把任务添加到任务队列

};