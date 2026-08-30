#include "Utils.h"

namespace Utils
{
    // 初始化控制台
    void init()
    {
#ifdef _WIN32
        // 检验是否重置
        if (!Exit::exit_event)
            // 手动重置
            Exit::exit_event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        // 设置控制台的编码格式
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCtrlHandler(Exit::ConsoleCtrlHandler, TRUE);
#else
        // Linux/macOS：注册信号处理器，Ctrl+C 或 kill 时触发优雅退出
        std::signal(SIGINT, Exit::Onsignal);
        std::signal(SIGTERM, Exit::Onsignal);
        // 可选：忽略 SIGPIPE 防止写入已关闭 socket 导致进程崩溃
        std::signal(SIGPIPE, SIG_IGN);
#endif
    }

    namespace Time
    {

        // 获取local
        void Get_Local(std::tm& local, time_t now)
        {
#if _WIN32
            localtime_s(&local, &now);
#else
            // 按照Linux编码的获取
            localtime_r(&now, &local);
#endif
        }

        // 获取现在时间
        std::string NowTime()
        {
            // 现在的时间的时间戳
            time_t now = std::time(nullptr);
            std::tm local{};

            // 获取时间
            Get_Local(local, now);

            std::ostringstream oss;
            oss << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
            return oss.str();
        }

        std::string NowDay()
        {
            // 现在的时间的时间戳
            auto now = std::time(nullptr);
            std::tm local{};

            // 获取时间
            Get_Local(local, now);

            // tm_year 从 1900 年开始算
            int year = local.tm_year + 1900;
            // tm_mon 范围是 0~11
            int month = local.tm_mon + 1;
            // 1~31
            int day = local.tm_mday;

            return std::to_string(year) + "-" + std::to_string(month) + "-" + std::to_string(day) + "-logs";
        }

    } // namespace Time
    // 退出
    namespace Exit
    {
        // 多个回调
        inline std::vector<std::function<void()>> stop_callbacks;
        inline std::mutex callbacks_mutex;

        // 注册回调
        void RegisterStopCallback(std::function<void()> cb)
        {
            std::lock_guard<std::mutex> lock(callbacks_mutex);
            stop_callbacks.push_back(std::move(cb));
        }

        // 结束的函数
        void GracefulShutdown()
        {
            bool expected = false;

            if (exit_called.compare_exchange_strong(expected, true))
            {
                Out::Out_Msg("收到退出信号，正在停止服务器...");

                exit_flag = true;

                running = false;

                // 遍历所有已注册的停止回调（每个服务只注册一个汇总回调）
                std::vector<std::function<void()>> callbacks;
                {
                    std::lock_guard<std::mutex> lock(callbacks_mutex);
                    callbacks = stop_callbacks;
                }
                for (auto& cb : callbacks)
                {
                    if (cb)
                        cb();
                }
            }

#ifdef _WIN32
            if (exit_event)
            {
                SetEvent(exit_event);
            }
#else
            {
                // Linux/macOS：通知 WaitExit() 返回
                std::lock_guard<std::mutex> lock(exit_mutex);
                exit_signaled = true;
            }
            exit_cv.notify_all();
#endif
        }

        // 等待退出
        void WaitExit()
        {
#ifdef _WIN32
            if (!exit_event)
                exit_event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
            WaitForSingleObject(exit_event, INFINITE);
#else
            while (running.load())
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif
        }

        // 收到退出的信号
        void RecviceExit()
        {
            GracefulShutdown();
        }

        // 按键退出的信号
        void Onsignal(int)
        {
            GracefulShutdown();
        }

#ifdef _WIN32
        BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType)
        {
            switch (ctrlType)
            {
            case CTRL_C_EVENT:
            case CTRL_BREAK_EVENT:
            case CTRL_CLOSE_EVENT:
            case CTRL_LOGOFF_EVENT:
            case CTRL_SHUTDOWN_EVENT:
                running = false;
                GracefulShutdown();
                return TRUE;
            default:
                return FALSE;
            }
        }
#endif
    } // namespace Exit

    // 打开文件
    namespace File
    {

        // 设置自定义的日志文件目录
        void SetLogsDir(const std::string dir)
        {
            logsdir = dir;
        }

        // 检查是否有logs文件夹，没有则创建
        bool CheckLogsDir()
        {
#ifdef _WIN32
            if (_mkdir(logsdir.c_str()) == 0)
                return true;
            return errno == EEXIST;
#else
            if (mkdir(dir.c_str(), 0755) == 0)
                return true;
            return errno == EEXIST;
#endif
        }

        // 追加
        bool Out_File_add(const std::string addr, const std::string msg)
        {
            // 默认打开模式是覆盖写
            std::ofstream out(addr.c_str(), std::ios::app);
            if (!out)
            {
                std::cerr << "打开文件失败" << std::endl;
                return 1;
            }
            out << msg << std::endl;
            out.close();
            return 0;
        }

        // 写入日志
        void Out_Log(const std::string msg)
        {
            // 确认是否有这个文件夹
            if (CheckLogsDir)
            {
                std::cerr << "创建logs失败" << std::endl;
            }
            std::string addr = logsdir + "/" + Time::NowDay() + ".txt";
            if (File::Out_File_add(addr, msg))
                std::cout << "写入日志失败" << std::endl;
        }
    } // namespace File

    namespace Out
    {
        // 正常输出
        void Out_Msg(const std::string msg)
        {
            std::string Out_Str = "[" + ServiceID[serviceID] + "][INFO]" + Time::NowTime() + " " + msg;
            std::cout << Out_Str << std::endl;
            File::Out_Log(Out_Str);
        }

        // 错误输出
        void Out_Err(const std::string msg)
        {
            std::string Out_Str = "[" + ServiceID[serviceID] + "][ERROR]" + Time::NowTime() + " " + msg;
            std::cerr << Out_Str << std::endl;
            File::Out_Log(Out_Str);
        }

        // 网络输出
        // 网络部分输出
        void Out_Net_Msg(unsigned long long msg_id, std::string msg)
        {
            Out_Msg("[信息ID:" + std::to_string(msg_id) + "]" + msg);
        }
    } // namespace Out
} // namespace Utils
