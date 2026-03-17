#include "path_util.h"
#include "shine_define.h"

#ifdef SHINE_PLATFORM_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <shlwapi.h>
#include <windows.h>
#pragma comment(lib, "shlwapi.lib")
#else
#include <cstdlib>
#include <limits.h>
#include <unistd.h>
#endif

#include <array>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <vector>

#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine::util {
std::optional<SString> get_executable_directory() {
#ifdef SHINE_PLATFORM_WIN
    std::array<char, MAX_PATH> buffer{};
    DWORD                      length = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size())
        return std::nullopt;

    PathRemoveFileSpecA(buffer.data());
    return SString::from_utf8(buffer.data());
#elif SHINE_PLATFORM_WASM
    return std::nullopt;
#else
    char    buffer[PATH_MAX];
    ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (length == -1)
        return std::nullopt;

    buffer[length] = '\0';

    SString exe_path = SString::from_utf8(buffer);
    size_t  last_slash = exe_path.find_last_of('/');
    if (last_slash == SString::npos)
        return std::nullopt;

    return exe_path.substr(0, last_slash);
#endif
}

std::optional<SString> get_script_path(const SString &script_name) {
    auto exe_dir = get_executable_directory();
    if (!exe_dir.has_value())
        return std::nullopt;

    std::filesystem::path exe_path(exe_dir->c_str());
    std::filesystem::path script_path = exe_path / ".." / "build" / "script" / script_name.c_str();

    try {
        script_path = std::filesystem::absolute(script_path);
        return SString::from_utf8(script_path.string());
    } catch (...) {
        return std::nullopt;
    }
}

SString normalize_path(const SString &path) {
    std::filesystem::path fsPath(path.c_str());
    std::string           result = fsPath.lexically_normal().string();
    std::ranges::replace(result, '/', '\\');
    return SString::from_utf8(result);
}

bool is_absolute_path(STextView path) {
    return std::filesystem::path(path.data(), path.data() + path.size()).is_absolute();
}

SString join_path(STextView base, STextView part) {
    if (base.empty()) return normalize_path(SString::from_view(part));
    if (part.empty()) return normalize_path(SString::from_view(base));
    if (is_absolute_path(part)) return normalize_path(SString::from_view(part));

    std::filesystem::path combined = std::filesystem::path(base.data(), base.data() + base.size()) /
                                     std::filesystem::path(part.data(), part.data() + part.size());
    return normalize_path(SString::from_utf8(combined.string()));
}   

SString normalize_asset_path(STextView path) {
    SString normalized = SString::from_view(path);
    std::ranges::replace(normalized, '\\', '/');

    std::ranges::transform(normalized, normalized.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    while (normalized.size() > 1 && normalized.back() == '/') {
        normalized.erase(normalized.size() - 1);
    }
    return normalized;
}

std::optional<SString> to_absolute_path(const SString &relative_path, STextView base_path) {
    SString effective_base = base_path.empty() ? get_executable_directory().value_or(SString()) : SString::from_view(base_path);
    if (effective_base.empty()) return std::nullopt;

    try {
        std::filesystem::path base(effective_base.c_str());
        std::filesystem::path relative(relative_path.c_str());
        std::filesystem::path absolute = std::filesystem::absolute(base / relative);
        return SString::from_utf8(absolute.string());
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<SString> split_path_components(STextView path) {
    std::vector<SString> components;
    size_t start = 0;
    while (start < path.size()) {
        size_t pos = path.find_first_of("/\\", start);
        if (pos == STextView::npos) {
            components.emplace_back(SString::from_view(path.substr(start, path.size() - start)));
            break;
        }
        if (pos > start) components.emplace_back(SString::from_view(path.substr(start, pos - start)));
        start = path.find_first_not_of("/\\", pos);
        if (start == STextView::npos) break;
    }
    return components;
}

STextView get_file_extension(STextView path) {
    size_t dotPos = path.find_last_of('.');
    size_t sepPos = path.find_last_of("/\\");
    if (dotPos != STextView::npos && (sepPos == STextView::npos || dotPos > sepPos)) {
        return path.substr(dotPos, path.size() - dotPos);
    }
    return {};
}

SString normalize_file_extension(STextView path) {
    STextView ext = get_file_extension(path);
    if (ext.empty()) return {};
    if (ext.front() == '.') ext = ext.substr(1, ext.size() - 1);
    SString result = SString::from_view(ext);
    std::ranges::transform(result, result.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

STextView get_directory(STextView path) {
    size_t sepPos = path.find_last_of("/\\");
    if (sepPos != STextView::npos) return path.substr(0, sepPos);
    return {};
}

SString trim_file_extension(STextView path) {
    STextView ext = get_file_extension(path);
    if (!ext.empty()) return SString::from_view(path.substr(0, path.size() - ext.size()));
    return SString::from_view(path);
}

SString get_file_name_without_extension(STextView path) {
    size_t sepPos = path.find_last_of("/\\");
    STextView filename = (sepPos == STextView::npos) ? path : path.substr(sepPos + 1, path.size() - sepPos - 1);
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != STextView::npos) return SString::from_view(filename.substr(0, dotPos));
    return SString::from_view(filename);
}

SString to_standard_path(STextView path) {
    SString result = SString::from_view(path);
    std::ranges::replace(result, '\\', '/');
    return result;
}

SString to_windows_path(STextView path) {
    SString result = SString::from_view(path);
    std::ranges::replace(result, '/', '\\');
    return result;
}

SString to_platform_path(STextView path) {
#ifdef SHINE_PLATFORM_WIN
    return to_windows_path(path);
#else
    return to_standard_path(path);
#endif
}
}
