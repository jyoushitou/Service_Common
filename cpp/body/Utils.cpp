#include "Utils.h"

namespace Utils
{
    void init()
    {
#ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
#endif
    }

    // 获取现在时间
    std::string NowTime()
    {
        // 现在的时间的时间戳
        auto now = std::time(nullptr);
        std::tm local{};
        localtime_s(&local, &now); // Linux/mac 用 localtime_r(&now, &local)
        std::ostringstream oss;
        oss << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    // 打开文件

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

    // 正常输出
    void Out_Msg(const std::string msg, const int MSID)
    {
        std::string Out_Msg = "[" + ServiceID[MSID] + "]" + NowTime() + "  " + msg;
        std::cout << Out_Msg << std::endl;
        if (Out_File_add("logs.txt", Out_Msg))
            std::cout << "写入日志失败" << std::endl;
    }

    // 错误输出
    void Out_Err(const std::string msg, const int MSID)
    {
        std::cerr << '[' << ServiceID[MSID] << ']' << msg << std::endl;
    }

    // 网络输出
    // 网络部分输出
    void Out_Net_Msg(unsigned long long msg_id, std::string msg, int MSID)
    {
        Out_Msg(std::to_string(msg_id) + ":" + msg, MSID);
    }
} // namespace Utils
