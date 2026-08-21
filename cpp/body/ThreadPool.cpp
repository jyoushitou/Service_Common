#include "ThreadPool.h"

namespace ThreadPool
{
    // 构造函数的实现并为stop初始化为0（false）
    ThreadPool::ThreadPool(size_t num_threads) : stop(0)
    {
        // 循环num_threads此，去创建num_threads的线程数
        for (size_t i = 0; i < num_threads; i++)
        {
            // workers内部通过匿名函数指向私有worker函数构造子线程
            workers.emplace_back([this] { this->worker(); });
        }
    }

    ThreadPool::~ThreadPool() // 析构函数
    {
        // 更改停止标识
        {
            std::unique_lock<std::mutex>(mtx); // 给mtx上锁
            stop = 1;
        }
        cv.notify_all();                         // 唤醒所有的阻塞线程
        for (std::thread& otherthread : workers) // 循环为未完成的线程
        {
            otherthread.join(); // 阻塞主线程使未完成的线程完成
        }
    }

    // 每个线程执行函数
    void ThreadPool::worker()
    {
        // 死循环
        while (1)
        {
            // 定义一临时存储异步任务的变量
            std::function<void()> task;
            {
                // 给mtx上锁
                std::unique_lock<std::mutex> lock(mtx);

                // 等待条件（是否是stop变量为false，或者任务队列是否为空）
                cv.wait(lock, [this] { return this->stop || !this->work_que.empty(); });

                // stop==1并且任务队列为空，则关闭线程
                if (stop && work_que.empty())
                {
                    return;
                }

                // 给队列上面的赋值给task（获得函数的签名）
                task = std::move(this->work_que.front());

                // 出队
                this->work_que.pop();
            }
            task(); // 执行task
        }
    }

    // 获取消息队列长度
    size_t ThreadPool::QueueSize()
    {
        std::lock_guard<std::mutex> lock(this->mtx);
        return work_que.size();
    }
} // namespace ThreadPool