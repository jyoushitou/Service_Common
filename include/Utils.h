#pragma once

#include <iostream>
#include <ctime>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>

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
    std::string NowTime();

    // 初始化
    void init();

    // 打开文件

    // 追加的
    bool Out_File_add(const std::string addr, const std::string msg);
    // 覆写
    bool Out_File_wirte(const std::string addr, const std::string msg);

    // 输出信息
    void Out_Msg(const std::string msg, const int MSID);
    // 错误信息
    void Out_Err(const std::string msg, const int MSID);

    // 网络输出
    // 网络部分输出
    void Out_Net_Msg(unsigned long long msg_id, std::string msg, int MSID);
} // namespace Utils