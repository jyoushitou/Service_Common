#pragma once

#include <thread>
#include <vector>
#include <mutex>
#include <functional>
#include <queue>
#include <future>

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
        auto enques(F&& f, Arg&&... arg) -> std::future<typename std::result_of<F(Arg...)>::type>;

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