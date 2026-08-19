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
#include <vector>

#include "message.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#endif

namespace Utils
{
    // 存储当前服务器ID
    extern int serviceID;

    // 获取当前时间
    std::string NowTime();

    // 初始化
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
    namespace OpenFile
    {
        // 追加的
        bool Out_File_add(const std::string addr, const std::string msg);
        // 覆写
        bool Out_File_wirte(const std::string addr);
    } // namespace OpenFile

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