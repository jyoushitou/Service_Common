#pragma once

#include <thread>
#include <vector>
#include <mutex>
#include <functional>
#include <queue>
#include <future>
#include <stdexcept>

namespace ThreadPool
{
    // 线程池
    class ThreadPool
    {
    public:
        // 唯一构造（默认创建4个线程）
        explicit ThreadPool(size_t num_threads = 4);

        // F为单个固定的模版参数，Arg为不固定数量的模版类型
        template <typename F, typename... Arg>
        // 线程池的使用（添加函数）
        auto enques(F&& f, Arg&&... arg) -> std::future<std::invoke_result_t<F, Arg...>>
        {
            using functype = std::invoke_result_t<F, Arg...>; // 获取得到的函数类型

            auto task = std::make_shared<std::packaged_task<functype()>>(
                std::bind(std::forward<F>(f), std::forward<Arg>(arg)...)); // 打包一个传入函数为异步任务

            std::future<functype> retfuture = task->get_future();        // 获取上一步打包的异步函数
            {                                                            // 定义锁的作用域避免死锁
                std::lock_guard<std::mutex> lock_mtx(this->mtx);         // 为线程中调用函数加一个智能锁
                if (stop)                                                // 判断是否线程池停止
                    throw std::runtime_error("error:ThreadPool on off"); // 抛出停止异常
                work_que.emplace([this, task]() { (*task)(); });         // 将异步任务的函数名解出并加入线程队列
            }
            cv.notify_one(); // 唤醒一个线程去执行

            return retfuture; // 返回异步执行的结果
        }

        // 析构函数
        ~ThreadPool();

    private:
        // 线程执行内容
        void worker();
        // 记录线程池是否停止
        bool stop;
        // 条件变量
        std::condition_variable cv;
        // 互斥锁
        std::mutex mtx;
        // 线程集合（线程池）
        std::vector<std::thread> workers;
        // 任务队列
        std::queue<std::function<void()>> work_que;
    };

} // namespace ThreadPool