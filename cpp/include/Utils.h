#pragma once

#include <iostream>
#include <ctime>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>

#include "Message.h"

#include <thread>
#include <sys/stat.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <direct.h>
#else
#include <csignal>
#include <sys/types.h>
#endif

namespace Utils
{
    // 存储当前服务器ID
    extern int serviceID;

    namespace Time
    {
        // 根据操作系统获取local
        void Get_Local(std::tm& local, time_t now);
        // 获取当前时间
        std::string NowTime();

        // 获取当期日期
        std::string NowDay();

    } // namespace Time

    // 初始化控制台，注册回调
    void init();

    // 退出
    namespace Exit
    {

        // 退出标志
        inline std::atomic<bool> exit_flag(false);
        // 防止多次调用
        inline std::atomic<bool> exit_called(false);
        // 运行标志
        inline std::atomic<bool> running(true);

#ifdef _WIN32
        // 统一退出事件：主线程 WaitForExit() 阻塞等待
        inline HANDLE exit_event = nullptr;

        // Windows 控制台关闭事件处理
        BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType);
#else
        // 条件变量的实现通知
        inline std::mutex exit_mutex;
        inline std::condition_variable exit_cv;
        inline bool exit_signaled = false;
#endif

        // 回调停止服务
        void RegisterStopCallback(std::function<void()> cb);

        // 统一退出函数
        void GracefulShutdown();

        // 按键处理函数
        void Onsignal(int);

        // 清理资源
        void CleanUp();

        // 阻塞等待退出信号
        void WaitExit();

        // 主动触发退出
        void RecviceExit();
    } // namespace Exit

    // 打开文件
    namespace File
    {
        // 日志的写入路径
        std::string logsdir = "logs";

        // 设置自定义的日志文件目录
        void SetLogsDir(const std::string dir);

        // 检查是否有logs文件夹，没有则创建
        bool CheckLogsDir();

        // 追加的
        bool Out_File_add(const std::string addr, const std::string msg);
        // 覆写
        bool Out_File_wirte(const std::string addr, const std::string msg);

        // 写入日志
        void Out_Log(const std::string msg);
    } // namespace File

    namespace Out
    {
        // 输出信息
        void Out_Msg(const std::string msg);
        // 错误信息
        void Out_Err(const std::string msg);

        // 网络输出

        // 网络部分输出
        void Out_Net_Msg(unsigned long long msg_id, std::string msg);
    } // namespace Out

} // namespace Utils