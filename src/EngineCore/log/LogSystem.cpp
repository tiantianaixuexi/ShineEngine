#include "LogSystem.h"
#include <ctime>
#include <iostream>

#ifdef SHINE_USE_MODULE
import shine.memory;
#else
#include "../../memory/memory.ixx"
#endif

namespace shine {

    LogSystem& LogSystem::Get() {
        static LogSystem instance;
        return instance;
    }

    LogSystem::LogSystem() {}
    LogSystem::~LogSystem() {}

    void LogSystem::AddLog(const std::string& category, LogLevel level, const std::string& message) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        // Tag log allocations as Core
        shine::co::MemoryScope scope(shine::co::MemoryTag::Core);

        auto now = std::chrono::system_clock::now();
        std::string timeStr;
        try {
             timeStr = fmt::format("{:%H:%M:%S}", now);
        } catch(...) {
             timeStr = "00:00:00";
        }

        m_Logs.push_back({ level, category, message, timeStr });

        // Console fallback
        const char* levelStr = "";
        switch (level) {
            case LogLevel::Info:  levelStr = "INFO"; break;
            case LogLevel::Warn:  levelStr = "WARN"; break;
            case LogLevel::Error: levelStr = "ERROR"; break;
            case LogLevel::Debug: levelStr = "DEBUG"; break;
        }

        std::cout << fmt::format("[{}] [{}/{}] {}", timeStr, category, levelStr, message) << std::endl;
    }

    void LogSystem::Clear() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Logs.clear();
    }
}
