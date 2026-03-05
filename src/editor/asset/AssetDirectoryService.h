#pragma once

#include <filesystem>
#include <unordered_map>
#include <vector>

#include "EngineCore/subsystem.h"
#include "string/shine_string.h"
#include "util/function/EventHandle.h"

namespace shine::util::watcher
{
    struct FileChangeEvent;
    class FileWatchService;
}
namespace shine::util
{
    class EngineDirectoryService;
}

namespace shine::editor::asset
{
    class AssetDirectoryService : public shine::Subsystem
    {
    public:
        bool Init(EngineContext& ctx) override;
        void Shutdown(EngineContext& ctx) override;

        void ForceRefresh();
        void RefreshIfDirty();

        const std::filesystem::path& GetContentRootPath() const { return contentRootPath_; }
        const std::vector<std::filesystem::path>& GetRootDirectories() const;
        const std::vector<std::filesystem::path>& GetChildDirectories(const std::filesystem::path& path) const;
        const std::vector<std::filesystem::directory_entry>& GetEntries(const std::filesystem::path& path) const;

    private:
        void RebuildCache();
        bool IsUnderContentRoot(const std::filesystem::path& path) const;

    private:
        std::filesystem::path contentRootPath_;
        util::watcher::FileWatchService* fileWatchService_ = nullptr;
        util::EngineDirectoryService* engineDirectoryService_ = nullptr;
        util::EventHandle<const util::watcher::FileChangeEvent&>::Handle fileWatchHandle_;
        bool dirty_ = true;
        std::vector<std::filesystem::path> rootDirectories_;
        std::unordered_map<std::filesystem::path, std::vector<std::filesystem::path>> childDirectoriesByPath_;
        std::unordered_map<std::filesystem::path, std::vector<std::filesystem::directory_entry>> entriesByPath_;
        std::vector<std::filesystem::path> emptyDirectories_;
        std::vector<std::filesystem::directory_entry> emptyEntries_;
    };
}
