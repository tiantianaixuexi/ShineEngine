#pragma once
#include "EngineCore/subsystem.h"
#include <vector>
#include <string>
#include <mutex>
#include "fmt/format.h"
#include "fmt/chrono.h"

namespace shine {

    enum class LogLevel {
        Info,
        Warn,
        Error,
        Debug
    };

    struct LogEntry {
        LogLevel level;
        std::string category;
        std::string message;
        std::string timestamp;
    };

    class LogSystem : public Subsystem {
    public:
        static LogSystem& Get();

        LogSystem();
        ~LogSystem() override;

        template<typename... Args>
        void Log(const char* category, LogLevel level, fmt::format_string<Args...> fmt, Args&&... args) {
            try {
                std::string msg = fmt::format(fmt, std::forward<Args>(args)...);
                AddLog(category, level, msg);
            } catch (const std::exception& e) {
                AddLog("LogSystem", LogLevel::Error, std::string("Log format error: ") + e.what());
            }
        }

        void AddLog(const std::string& category, LogLevel level, const std::string& message);
        void Clear();
        const std::vector<LogEntry>& GetLogs() const { return m_Logs; }
        std::mutex& GetMutex() { return m_Mutex; }

    private:
        std::vector<LogEntry> m_Logs;
        mutable std::mutex m_Mutex;
    };

    // Helper macro to define a log category
    #define DEFINE_LOG_CATEGORY(CategoryName) \
        static constexpr const char* CategoryName = #CategoryName;

    // Helper macro to declare a log category (if needed in header)
    #define DECLARE_LOG_CATEGORY_EXTERN(CategoryName) \
        extern const char* CategoryName;
}

// Macros
#define SHINE_LOG(Category, Verbosity, ...) \
    shine::LogSystem::Get().Log(Category, shine::LogLevel::Verbosity, __VA_ARGS__)

// Legacy Macros (Default to "LogTemp" category)
#define SHINE_LOG_INFO(...)  SHINE_LOG("LogTemp", Info, __VA_ARGS__)
#define SHINE_LOG_WARN(...)  SHINE_LOG("LogTemp", Warn, __VA_ARGS__)
#define SHINE_LOG_ERROR(...) SHINE_LOG("LogTemp", Error, __VA_ARGS__)
#define SHINE_LOG_DEBUG(...) SHINE_LOG("LogTemp", Debug, __VA_ARGS__)
