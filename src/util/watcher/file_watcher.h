#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlwapi.h>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>


#include "util/function/EventHandle.h"
#include "util/encoding_util.ixx"
#include "fmt/xchar.h"

#pragma comment(lib, "shlwapi.lib")

namespace shine::util::watcher {

class DirectoryWatcher {
public:
    // path, filename, action, is_directory
    EventHandle<std::wstring, std::wstring, DWORD, bool> OnFileChanged;

private:
    struct IoContext {
        HANDLE dir_handle = INVALID_HANDLE_VALUE;
        std::wstring path;
        OVERLAPPED overlapped = {};
        std::vector<BYTE> buffer;
    };

    HANDLE iocp_ = INVALID_HANDLE_VALUE;
    std::atomic<bool> running_{false};
    std::mutex contexts_mutex;
    std::thread worker_thread;
    std::vector<IoContext*> contexts;

public:
    explicit DirectoryWatcher() = default;

    ~DirectoryWatcher() {
        stop();
    }

    bool CreateIocp() {
        if (iocp_ != INVALID_HANDLE_VALUE) {
            return true;
        }
        
        iocp_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
        if (iocp_ == nullptr) {
            fmt::println("CreateIoCompletionPort 失败: {}", GetLastError());
            iocp_ = INVALID_HANDLE_VALUE;
            return false;
        }
        return true;
    }

    bool AddWatchDirectory(const std::wstring& path) {
        if (iocp_ == INVALID_HANDLE_VALUE) {
            fmt::println("错误: IOCP 未创建");
            return false;
        }

        if (path.empty()) {
            fmt::println(L"错误: 路径为空");
            return false;
        }

        DWORD attr = GetFileAttributesW(path.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) {
            fmt::println(L"错误: 路径不存在: {} (错误码: {})", path, GetLastError());
            return false;
        }

        if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            fmt::println(L"错误: 路径不是目录: {}", path);
            return false;
        }

        if (!PathIsDirectoryW(path.c_str())) {
            fmt::println(L"错误: 无法访问目录: {}", path);
            return false;
        }

        HANDLE dir_handle = CreateFileW(
            path.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            nullptr
        );

        if (dir_handle == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            fmt::println(L"错误: 无法打开目录: {} (错误码: {})", path, error);
            return false;
        }

        IoContext* context = nullptr;
        try {
            context = new IoContext();
            context->dir_handle = dir_handle;
            context->path = path;
            context->buffer.reserve(4096);
        } catch (const std::bad_alloc&) {
            fmt::println("错误: 内存分配失败");
            CloseHandle(dir_handle);
            return false;
        }

        if (!CreateIoCompletionPort(dir_handle, iocp_, 
                                    reinterpret_cast<ULONG_PTR>(context), 0)) {
            DWORD error = GetLastError();
            fmt::println("错误: CreateIoCompletionPort 失败: {}", error);
            delete context;
            CloseHandle(dir_handle);
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(contexts_mutex);
            contexts.push_back(context);
        }

        if (!issue_read(context)) {
            fmt::println(L"错误: 无法开始监视目录: {}", path);
            remove_directory_internal(context);
            return false;
        }

        fmt::println("成功添加监视: {}",util::EncodingUtil::WstringToUTF8(path));
        return true;
    }

    void stop() {
        if (!running_.exchange(false)) {
            return;
        }

        if (iocp_ != INVALID_HANDLE_VALUE) {
            PostQueuedCompletionStatus(iocp_, 0, 0, nullptr);
        }

        if (worker_thread.joinable()) {
            worker_thread.join();
        }

        if (iocp_ != INVALID_HANDLE_VALUE) {
            CloseHandle(iocp_);
            iocp_ = INVALID_HANDLE_VALUE;
        }

        std::lock_guard<std::mutex> lock(contexts_mutex);
        for (auto* ctx : contexts) {
            if (ctx->dir_handle != INVALID_HANDLE_VALUE) {
                CancelIo(ctx->dir_handle);
                CloseHandle(ctx->dir_handle);
            }
            delete ctx;
        }
        contexts.clear();
        fmt::println("目录监视器已停止");
    }

    void start_async() {
        if (running_.exchange(true)) {
            fmt::println("警告: 监视器已经在运行");
            return;
        }

        if (iocp_ == INVALID_HANDLE_VALUE) {
            fmt::println("错误: IOCP 未创建");
            running_ = false;
            return;
        }

        worker_thread = std::thread([this]() {
            run();
        });
        fmt::println("监视器已在后台启动");
    }

    bool is_running() const {
        return running_;
    }

    void run() {
        fmt::println("IOCP 事件循环开始");

        while (running_) {
            DWORD bytes_transferred = 0;
            ULONG_PTR completion_key = 0;
            OVERLAPPED* overlapped = nullptr;

            BOOL success = GetQueuedCompletionStatus(
                iocp_,
                &bytes_transferred,
                &completion_key,
                &overlapped,
                1000
            );

            if (!running_) {
                break;
            }

            if (!success && !overlapped) {
                continue;
            }

            if (!success) {
                DWORD error = GetLastError();
                
                if (completion_key != 0 && overlapped) {
                    auto* context = reinterpret_cast<IoContext*>(completion_key);
                    fmt::println(L"IOCP 错误 (目录: {}): {}", 
                                context->path, error);
                    
                    if (running_ && context->dir_handle != INVALID_HANDLE_VALUE) {
                        if (!issue_read(context)) {
                            fmt::println(L"重新监视失败，移除目录: {}", context->path);
                            std::thread([this, context]() {
                                remove_directory(context);
                            }).detach();
                        }
                    }
                } else {
                    fmt::println("IOCP 未知错误: {}", error);
                }
                continue;
            }

            if (completion_key == 0 || !overlapped) {
                continue;
            }

            auto* context = reinterpret_cast<IoContext*>(completion_key);
            
            {
                std::lock_guard<std::mutex> lock(contexts_mutex);
                auto it = std::find(contexts.begin(), contexts.end(), context);
                if (it == contexts.end()) {
                    fmt::println("警告: 收到未知上下文的事件");
                    continue;
                }
            }

            if (bytes_transferred > 0) {
                ProcessNotifications(context, bytes_transferred);
            }

            if (running_ && context->dir_handle != INVALID_HANDLE_VALUE) {
                if (!issue_read(context)) {
                    fmt::println(L"重新监视失败，目录可能已移除: {}", context->path);
                }
            }
        }

        fmt::println("IOCP 事件循环结束");
    }

private:
    void remove_directory_internal(IoContext* context) {
        if (!context) return;

        if (context->dir_handle != INVALID_HANDLE_VALUE) {
            CancelIo(context->dir_handle);
            CloseHandle(context->dir_handle);
            context->dir_handle = INVALID_HANDLE_VALUE;
        }

        auto it = std::find(contexts.begin(), contexts.end(), context);
        if (it != contexts.end()) {
            contexts.erase(it);
        }
        delete context;
    }

    void remove_directory(IoContext* context) {
        std::lock_guard<std::mutex> lock(contexts_mutex);
        remove_directory_internal(context);
    }

    bool issue_read(IoContext* context) {
        if (!context || context->dir_handle == INVALID_HANDLE_VALUE) {
            return false;
        }

        context->buffer.resize(4096);
        ZeroMemory(&context->overlapped, sizeof(OVERLAPPED));

        BOOL success = ReadDirectoryChangesW(
            context->dir_handle,
            context->buffer.data(),
            static_cast<DWORD>(context->buffer.size()),
            TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME |
                FILE_NOTIFY_CHANGE_DIR_NAME |
                FILE_NOTIFY_CHANGE_ATTRIBUTES |
                FILE_NOTIFY_CHANGE_SIZE |
                FILE_NOTIFY_CHANGE_LAST_WRITE |
                FILE_NOTIFY_CHANGE_CREATION,
            nullptr,
            &context->overlapped,
            nullptr
        );

        if (!success) {
            DWORD error = GetLastError();
            if (error != ERROR_IO_PENDING) {
                fmt::println(L"ReadDirectoryChangesW 失败 (目录: {}): {}", 
                            context->path, error);
                return false;
            }
        }
        return true;
    }

    // 关键函数：判断路径是文件还是目录
    static bool IsDirectory(const std::wstring& full_path) {
        DWORD attr = GetFileAttributesW(full_path.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) {
            // 文件可能被快速删除，尝试从父目录推断
            // 或者根据事件类型猜测（重命名的旧名称无法检查）
            return false;
        }
        return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    void ProcessNotifications(IoContext* context, DWORD length) {
        if (!context || length == 0) return;

        FILE_NOTIFY_INFORMATION* info =
            reinterpret_cast<FILE_NOTIFY_INFORMATION*>(context->buffer.data());

        while (true) {
            std::wstring filename;
            try {
                filename = std::wstring(info->FileName, info->FileNameLength / sizeof(WCHAR));
            } catch (...) {
                fmt::println("警告: 文件名构造失败");
                break;
            }

            // 构建完整路径
            std::wstring full_path = context->path;
            if (!full_path.empty() && full_path.back() != L'\\' && full_path.back() != L'/') {
                full_path += L'\\';
            }
            full_path += filename;

            // 判断是文件还是目录
            // 注意：对于 FILE_ACTION_REMOVED 和 FILE_ACTION_RENAMED_OLD_NAME
            // 文件可能已经被删除，GetFileAttributes 会失败
            bool is_dir = false;
            if (info->Action == FILE_ACTION_REMOVED || 
                info->Action == FILE_ACTION_RENAMED_OLD_NAME) {
                // 已删除的文件/目录，尝试检查，失败则默认为文件
                is_dir = IsDirectory(full_path);
            } else {
                is_dir = IsDirectory(full_path);
            }

            // 触发事件，增加 is_directory 参数
            OnFileChanged.emit(context->path, filename, info->Action, is_dir);

            if (info->NextEntryOffset == 0)
                break;
                
            BYTE* next = reinterpret_cast<BYTE*>(info) + info->NextEntryOffset;
            if (next >= context->buffer.data() + context->buffer.size()) {
                fmt::println("警告: 通知数据越界");
                break;
            }
            info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(next);
        }
    }
};

} // namespace shine::util::watcher