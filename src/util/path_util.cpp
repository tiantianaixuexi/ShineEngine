#undef SHINE_USE_MODULE
#include "path_util.h"
#include "shine_define.h"
#include "string_util.ixx"

#ifdef SHINE_PLATFORM_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#elif SHINE_PLATFORM_WASM
#include <emscripten.h>
#else
#include <unistd.h>
#include <limits.h>
#include <cstdlib>
#endif

#include <string>
#include <vector>
#include <filesystem>

namespace shine::util
{
    std::optional<std::string> get_executable_directory()
    {
#ifdef SHINE_PLATFORM_WIN
        char buffer[MAX_PATH];
        DWORD length = GetModuleFileNameA(NULL, buffer, MAX_PATH);
        if (length == 0 || length == MAX_PATH)
            return std::nullopt;

        // 获取目录路径
        PathRemoveFileSpecA(buffer);
        return std::string(buffer);
#elif SHINE_PLATFORM_WASM
        // WASM环境下，返回空或固定路径
        return std::nullopt;
#else
        char buffer[PATH_MAX];
        ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        if (length == -1)
            return std::nullopt;

        buffer[length] = '\0';

        // 获取目录路径
        std::string exe_path(buffer);
        size_t last_slash = exe_path.find_last_of('/');
        if (last_slash == std::string::npos)
            return std::nullopt;

        return exe_path.substr(0, last_slash);
#endif
    }

    std::optional<std::string> get_script_path(const std::string& script_name)
    {
        auto exe_dir = get_executable_directory();
        if (!exe_dir.has_value())
            return std::nullopt;

        // 从 exe 目录向上查找 build/script/ 目录
        std::filesystem::path exe_path(exe_dir.value());
        std::filesystem::path script_path = exe_path / ".." / "build" / "script" / script_name;

        // 规范化路径
        try {
            script_path = std::filesystem::absolute(script_path);
            return script_path.string();
        }
        catch (...) {
            return std::nullopt;
        }
    }

    std::string normalize_path(const std::string& path)
    {
#ifdef SHINE_PLATFORM_WIN
        // Windows 下将 '/' 转换为 '\'
        std::string result = path;
        std::replace(result.begin(), result.end(), '/', '\\');
        return result;
#else
        // 其他平台保持 '/'
        return path;
#endif
    }

    std::optional<std::string> to_absolute_path(const std::string& relative_path, const std::string& base_path)
    {
        std::string effective_base = base_path.empty() ?
            get_executable_directory().value_or("") : base_path;

        if (effective_base.empty())
            return std::nullopt;

        try {
            std::filesystem::path base(effective_base);
            std::filesystem::path relative(relative_path);
            std::filesystem::path absolute = base / relative;
            absolute = std::filesystem::absolute(absolute);
            return absolute.string();
        }
        catch (...) {
            return std::nullopt;
        }
    }
}
