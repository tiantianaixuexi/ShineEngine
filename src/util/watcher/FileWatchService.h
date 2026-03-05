#pragma once

#include <filesystem>
#include <mutex>
#include <vector>

#include "EngineCore/subsystem.h"
#include "util/function/EventHandle.h"
#include "util/watcher/file_watcher.h"

namespace shine::util
{
    class EngineDirectoryService;
}

namespace shine::util::watcher
{
    struct FileChangeEvent
    {
        std::wstring directory;
        std::wstring filename;
        DWORD action = 0;
        bool isDirectory = false;
    };

    class FileWatchService : public shine::Subsystem
    {
    public:
        EventHandle<const FileChangeEvent&> OnFileChanged;

        bool Init(EngineContext& ctx) override;
        void Shutdown(EngineContext& ctx) override;

        bool AddWatchDirectory(const std::filesystem::path& path);
        void PumpEvents();
        const std::filesystem::path& GetDefaultContentRoot() const { return contentRoot_; }

    private:
        void EnqueueEvent(const std::wstring& directory, const std::wstring& filename, DWORD action, bool isDirectory);

    private:
        DirectoryWatcher watcher_;
        EventHandle<std::wstring, std::wstring, DWORD, bool>::Handle watcherBindHandle_;
        std::mutex pendingMutex_;
        std::vector<FileChangeEvent> pendingEvents_;
        std::filesystem::path contentRoot_;
        util::EngineDirectoryService* engineDirectoryService_ = nullptr;
    };
}
