#include"ThreadPool.h"

ThreadPool::ThreadPool(size_t threadnum,std::string threadtype):stop_(false),threadtype_(threadtype)
{
    for(size_t ii = 0; ii < threadnum; ii++)
    {
        //用lambda函数创建线程，emplace_back()函数可在容器内直接构造元素,整个lambda里都可以理解为线程的执行函数
        threads_.emplace_back([this]
        {
            printf("create %s thread(%d).\n",threadtype_.c_str(),(int)syscall(SYS_gettid));     //显示线程id
            //std::cout << "子线程：" << std::this_thread::get_id() << std::endl;   //C11标准线程id

            while(stop_ != true)
            {
                std::function<void()> task;
                {   //锁作用域的开始
                    std::unique_lock<std::mutex> lock(this->mutex_);        //unique_lock灵活的锁应用，可手动上锁，和信号量配合，出作用域也释放

                    //等待生产者的条件变量,如果线程都退出或任务队列非空就触发条件变量
                    this->condition_.wait(lock,[this]
                    {
                        return ((this->stop_ == true) || (this->taskqueue_.empty() == false));
                    });

                    //在线程停止前，如果队列中还有任务，则执行完再退出
                    if((this->stop_ == true) && (this->taskqueue_.empty() == true))return;      //线程都已结束或无任务则退出线程
                    
                    //出列一个任务,std::function不可拷贝，move-only只能移动
                    task = std::move(this->taskqueue_.front());
                    this->taskqueue_.pop();
                }

                printf("%s(%d)execute task\n",threadtype_.c_str(),(int)syscall(SYS_gettid));
                //std::cout << "子线程：" << std::this_thread::get_id() << std::endl;
                task();     //执行任务
            }

        });
    }
}

ThreadPool::~ThreadPool()
{
    stop_ = true;
    condition_.notify_all();    //唤醒全部的线程，只有唤醒后检测到无任务才会退出/释放

    //等待全部的线程执行完任务再退出
    for(std::thread &th:threads_)
    {
        th.join();
    }

}

void ThreadPool::addtask(std::function<void()> task)
{
    {   //锁的作用域开始
        std::lock_guard<std::mutex> lock(mutex_);       //lock_guard简单的锁管理只用构造时上锁，出作用域后自动解锁
        taskqueue_.push(task);       
    }
    
    condition_.notify_one();    //唤醒一个线程
}

//  测试线程池
// void show(int no,const std::string &name)
// {
//     printf("你好，我是%d号%s\n",no,name.c_str());
// }

// void test()
// {
//     printf("我是小鸟\n");
// }

// int main()
// {
//     ThreadPool threadpool(3);

//     std::string name = "苹果";
//     threadpool.addtask(std::bind(show,8,name));
//     sleep(1);

//     threadpool.addtask(std::bind(test));
//     sleep(1);

//     threadpool.addtask(std::bind([]{
//         printf("我是大鸟\n");
//     }));
//     sleep(1);

// }

//g++ -o test ThreadPool.cpp -lpthread