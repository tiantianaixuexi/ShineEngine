#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "constexpr/constexpr_str.h"
#include "fmt/chrono.h"
#include "fmt/format.h"
#include "util/EnumFlags.h"

namespace shine::log {

enum class LogCategoryFlag : std::uint32_t {
    E_WRITE_TO_FILE    = 1 << 0,
    E_WRITE_TO_CONSOLE = 1 << 1,
    E_WRITE_TO_ALL     = 0xFFFFFFFF
};

};

ENABLE_ENUM_FLAGS(shine::log::LogCategoryFlag)

namespace shine {

enum ShineLogLevel {
    Info,
    Warn,
    Error,
    Debug
};

constexpr const char *ShineLogLevelToString(ShineLogLevel level) noexcept {
    switch (level) {
    case ShineLogLevel::Info:
        return "INFO";
    case ShineLogLevel::Warn:
        return "WARN";
    case ShineLogLevel::Error:
        return "ERROR";
    case ShineLogLevel::Debug:
        return "DEBUG";
    default:
        return "UNKNOWN";
    }
}

struct CategoryConfig {
    log::LogCategoryFlag flags;
};

struct LogEntry {
    ShineLogLevel level;
    std::string   message;
};

class ILogImple {
public:
    virtual ~ILogImple() = default;
    virtual void AddLog(const std::string &category, ShineLogLevel level, const std::string &message);

    std::vector<LogEntry> m_Logs;
};

class LogImple : public ILogImple {
public:
    // 添加组
    void AddCategory(const std::string &category, CategoryConfig config = {log::LogCategoryFlag::E_WRITE_TO_CONSOLE}) noexcept {
        m_Categories.emplace(category, config);
    }
    void AddCategory(const std::string &category, bool writeToConsole) noexcept {
        AddCategory(category, {writeToConsole ? log::LogCategoryFlag::E_WRITE_TO_CONSOLE : log::LogCategoryFlag::E_WRITE_TO_FILE});
    }

    template <typename... Args>
    void Log(const char *category, ShineLogLevel level, fmt::format_string<Args...> fmt, Args &&...args) {

        const std::string &msg = fmt::format(fmt, std::forward<Args>(args)...);
        const size_t sizeBefore = m_Logs.size();
        AddLog(category, level, msg);
        if (ShouldWriteToConsole(category) && m_Logs.size() > sizeBefore) {
            fmt::print("{}", m_Logs.back().message);
        }
    }

    template <typename... Args>
    void AddCategorys(Args &&...args) noexcept {
        (AddCategory(std::forward<Args>(args)), ...);
    }

private:
    bool ShouldWriteToConsole(const std::string &category) const noexcept {
        auto it = m_Categories.find(category);
        if (it == m_Categories.end()) {
            return true;
        }
        return HasAnyFlag(it->second.flags, log::LogCategoryFlag::E_WRITE_TO_CONSOLE);
    }
    std::unordered_map<std::string, CategoryConfig> m_Categories;
};

class ShineLogManager {
public:
    ShineLogManager() {};
    ~ShineLogManager() {};

    static ShineLogManager &Get();

    void RegisterGroud(const std::string &_str, LogImple *ptr) {
        std::lock_guard<std::mutex> lock(m_RegisterMutex);
        if (m_LogGroups.contains(_str)) {
            // 已经注册过了
            fmt::println(stderr, "Log group '{}' is already registered", _str);
            return;
        }

        m_LogGroups[_str] = ptr;
    }

    const std::unordered_map<std::string, LogImple *> &GetLogGroups() const noexcept {
        return m_LogGroups;
    }

    const std::vector<LogEntry> &GetLogs() const noexcept {
        return m_Logs;
    }

    void PushLog(const LogEntry &log) {
        m_Logs.push_back(log);
    }

private:
    std::unordered_map<std::string, LogImple *>  m_LogGroups;
    std::mutex                                   m_RegisterMutex;
    std::vector<LogEntry>                     m_Logs;
};

} // namespace shine

#define REGISTER_LOG_GROUP(GroupName)                               \
    class GroupName##_Imple : public shine::LogImple {             \
    public:                                                         \
        GroupName##_Imple() {                                       \
            ShineLogManager::Get().RegisterGroud(#GroupName, this); \
        }                                                           \
        static GroupName##_Imple &getInstance();                    \
    };

#define REGISTER_LOG_GROUP_END(GroupName)                 \
    GroupName##_Imple &GroupName##_Imple::getInstance() { \
        static GroupName##_Imple instance;                \
        return instance;                                  \
    };

#define ADD_LOG_CATEGORY(GroupName, CategoryName) \
    GroupName##_Imple::getInstance().AddCategory(CategoryName);

#define ADD_LOG_CATEGORY_WITH_CONFIG(GroupName, CategoryName, Config) \
    GroupName##_Imple::getInstance().AddCategory(CategoryName, Config);

#define ADD_LOG_CATEGORY_WITH_CONSOLE(GroupName, CategoryName, WriteToConsole) \
    GroupName##_Imple::getInstance().AddCategory(CategoryName, WriteToConsole);

#define ADD_LOG_CATEGORYS(GroupName, ...) \
    GroupName##_Imple::getInstance().AddCategorys(__VA_ARGS__);

// Macros
#define SHINE_LOG(LogName,Category, Verbosity, ...) \
    shine::ShineLogManager::Get().GetLogGroups().at(#LogName)->Log(Category, shine::ShineLogLevel::Verbosity, __VA_ARGS__)

// Legacy Macros (Default to "LogTemp" category)
#define SHINE_LOG_INFO(LogName, Category, ...) SHINE_LOG(LogName, Category, Info, __VA_ARGS__)
#define SHINE_LOG_WARN(LogName, Category, ...) SHINE_LOG(LogName, Category, Warn, __VA_ARGS__)
#define SHINE_LOG_ERROR(LogName, Category, ...) SHINE_LOG(LogName, Category, Error, __VA_ARGS__)
#define SHINE_LOG_DEBUG(LogName, Category, ...) SHINE_LOG(LogName, Category, Debug, __VA_ARGS__)
