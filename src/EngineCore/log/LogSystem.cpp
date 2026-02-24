#include "LogSystem.h"
#include <ctime>
#include <iostream>

#ifdef SHINE_USE_MODULE
import shine.memory;
#else
#include "../../memory/memory.ixx"
#endif

namespace shine {

    ShineLogManager& ShineLogManager::Get() {
        static ShineLogManager instance;
        return instance;
    }



    void ILogImple::AddLog(const std::string& category, ShineLogLevel level, const std::string& message) {

        auto now = std::chrono::system_clock::now();
        // 去掉毫秒，截断到秒再格式化，避免输出小数秒
        auto now_s = std::chrono::time_point_cast<std::chrono::seconds>(now);
        std::time_t tt = std::chrono::system_clock::to_time_t(now_s);
        std::tm local_tm{};
    #if defined(_WIN32) || defined(_WIN64)
        localtime_s(&local_tm, &tt);
    #else
        localtime_r(&tt, &local_tm);
    #endif
        std::string timeStr = fmt::format("{:%Y-%m-%d %H:%M:%S}", local_tm);

        m_Logs.push_back({level,fmt::format("[{}] [{}] [{}]  {}", category, ShineLogLevelToString(level),timeStr, message)});
    }

}
