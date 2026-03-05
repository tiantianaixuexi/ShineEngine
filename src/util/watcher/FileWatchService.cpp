#include "util/watcher/FileWatchService.h"

#include "EngineCore/engine_context.h"
#include "fmt/base.h"
#include "util/EngineDirectoryService.h"

namespace shine::util::watcher
{
    bool FileWatchService::Init(EngineContext& ctx)
    {
        if (!watcher_.CreateIocp())
        {
            return false;
        }

        watcherBindHandle_ = watcher_.OnFileChanged.bind([this](std::wstring directory, std::wstring filename, DWORD action, bool isDirectory) {
            EnqueueEvent(directory, filename, action, isDirectory);
        });

        engineDirectoryService_ = ctx.GetSystem<util::EngineDirectoryService>();
        if (!engineDirectoryService_)
        {
            fmt::println("FileWatchService: EngineDirectoryService 未注册");
            return false;
        }
        contentRoot_ = engineDirectoryService_->GetContentDirectory();
        if (!AddWatchDirectory(contentRoot_))
        {
            fmt::println("FileWatchService: 默认监控目录添加失败: {}", contentRoot_.string());
        }

        watcher_.start_async();
        return true;
    }

    void FileWatchService::Shutdown(EngineContext& ctx)
    {
        (void)ctx;
        if (watcherBindHandle_)
        {
            watcher_.OnFileChanged.unbind(watcherBindHandle_);
            watcherBindHandle_ = {};
        }
        watcher_.stop();
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            pendingEvents_.clear();
        }
        engineDirectoryService_ = nullptr;
        OnFileChanged.clear();
    }

    bool FileWatchService::AddWatchDirectory(const std::filesystem::path& path)
    {
        const auto normalized = std::filesystem::absolute(path);
        return watcher_.AddWatchDirectory(normalized.wstring());
    }

    void FileWatchService::PumpEvents()
    {
        std::vector<FileChangeEvent> pending;
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            pending.swap(pendingEvents_);
        }

        for (const auto& event : pending)
        {
            OnFileChanged.emit(event);
        }
    }

    void FileWatchService::EnqueueEvent(const std::wstring& directory, const std::wstring& filename, DWORD action, bool isDirectory)
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pendingEvents_.push_back(FileChangeEvent{
            .directory = directory,
            .filename = filename,
            .action = action,
            .isDirectory = isDirectory
        });
    }
}
