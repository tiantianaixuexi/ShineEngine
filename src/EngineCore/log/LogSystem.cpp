#include "LogSystem.h"
#include "util/timer/timer_util.h"


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

        auto t = util::now_truncated();
        std::string timeStr = util::format_seconds(t);
        std::string msg = fmt::format("[{}] [{}] [{}]  {}\n", category, ShineLogLevelToString(level), timeStr, message);
        m_Logs.push_back({level, msg});
        ShineLogManager::Get().PushLog({level, msg});
    }

    

}
