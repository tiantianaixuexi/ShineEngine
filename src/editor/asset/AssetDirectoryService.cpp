#include "editor/asset/AssetDirectoryService.h"

#include <algorithm>
#include <cwctype>
#include <functional>

#include "EngineCore/engine_context.h"
#include "fmt/base.h"
#include "util/EngineDirectoryService.h"
#include "util/watcher/FileWatchService.h"

namespace shine::editor::asset
{
    bool AssetDirectoryService::Init(EngineContext& ctx)
    {
        engineDirectoryService_ = ctx.GetSystem<util::EngineDirectoryService>();
        if (!engineDirectoryService_)
        {
            fmt::println("AssetDirectoryService: EngineDirectoryService 未注册");
            return false;
        }
        contentRootPath_ = engineDirectoryService_->GetContentDirectory();
        std::error_code ec;
        std::filesystem::create_directories(contentRootPath_, ec);

        fileWatchService_ = ctx.GetSystem<util::watcher::FileWatchService>();
        if (fileWatchService_)
        {
            fileWatchHandle_ = fileWatchService_->OnFileChanged.bind([this](const util::watcher::FileChangeEvent& event) {
                const auto changedPath = std::filesystem::path(event.directory) / event.filename;
                if (IsUnderContentRoot(changedPath))
                {
                    dirty_ = true;
                }
            });
        }

        RebuildCache();
        return true;
    }

    void AssetDirectoryService::Shutdown(EngineContext& ctx)
    {
        (void)ctx;
        if (fileWatchService_ && fileWatchHandle_)
        {
            fileWatchService_->OnFileChanged.unbind(fileWatchHandle_);
            fileWatchHandle_ = {};
        }
        fileWatchService_ = nullptr;
        engineDirectoryService_ = nullptr;
        rootDirectories_.clear();
        childDirectoriesByPath_.clear();
        entriesByPath_.clear();
    }

    void AssetDirectoryService::ForceRefresh()
    {
        dirty_ = true;
    }

    void AssetDirectoryService::RefreshIfDirty()
    {
        if (!dirty_)
        {
            return;
        }
        RebuildCache();
    }

    const std::vector<std::filesystem::path>& AssetDirectoryService::GetRootDirectories() const
    {
        return rootDirectories_;
    }

    const std::vector<std::filesystem::path>& AssetDirectoryService::GetChildDirectories(const std::filesystem::path& path) const
    {
        if (const auto it = childDirectoriesByPath_.find(path); it != childDirectoriesByPath_.end())
        {
            return it->second;
        }
        return emptyDirectories_;
    }

    const std::vector<std::filesystem::directory_entry>& AssetDirectoryService::GetEntries(const std::filesystem::path& path) const
    {
        if (const auto it = entriesByPath_.find(path); it != entriesByPath_.end())
        {
            return it->second;
        }
        return emptyEntries_;
    }

    void AssetDirectoryService::RebuildCache()
    {
        dirty_ = false;
        rootDirectories_.clear();
        childDirectoriesByPath_.clear();
        entriesByPath_.clear();

        std::error_code ec;
        if (!std::filesystem::exists(contentRootPath_, ec) || !std::filesystem::is_directory(contentRootPath_, ec))
        {
            return;
        }

        std::function<void(const std::filesystem::path&)> buildDirectoryNode = [&](const std::filesystem::path& directoryPath) {
            auto& entries = entriesByPath_[directoryPath];
            auto& children = childDirectoriesByPath_[directoryPath];

            for (const auto& entry : std::filesystem::directory_iterator(directoryPath, ec))
            {
                if (ec)
                {
                    break;
                }
                entries.push_back(entry);
                if (entry.is_directory(ec))
                {
                    children.push_back(entry.path());
                    buildDirectoryNode(entry.path());
                }
            }

            std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
                std::error_code sec;
                const bool leftIsDir = lhs.is_directory(sec);
                const bool rightIsDir = rhs.is_directory(sec);
                if (leftIsDir != rightIsDir)
                {
                    return leftIsDir && !rightIsDir;
                }
                return lhs.path().filename().string() < rhs.path().filename().string();
            });
            std::sort(children.begin(), children.end());
        };

        for (const auto& entry : std::filesystem::directory_iterator(contentRootPath_, ec))
        {
            if (ec)
            {
                break;
            }
            if (!entry.is_directory(ec))
            {
                continue;
            }
            rootDirectories_.push_back(entry.path());
            buildDirectoryNode(entry.path());
        }
        entriesByPath_[contentRootPath_] = {};
        for (const auto& entry : std::filesystem::directory_iterator(contentRootPath_, ec))
        {
            if (ec)
            {
                break;
            }
            entriesByPath_[contentRootPath_].push_back(entry);
        }
        childDirectoriesByPath_[contentRootPath_] = rootDirectories_;
        std::sort(rootDirectories_.begin(), rootDirectories_.end());
    }

    bool AssetDirectoryService::IsUnderContentRoot(const std::filesystem::path& path) const
    {
        if (contentRootPath_.empty())
        {
            return false;
        }
        std::error_code ec;
        const auto fullPath = std::filesystem::weakly_canonical(path, ec);
        const auto rootPath = std::filesystem::weakly_canonical(contentRootPath_, ec);
        if (ec)
        {
            return false;
        }
        const auto fullStr = fullPath.wstring();
        const auto rootStr = rootPath.wstring();
        if (fullStr.size() < rootStr.size())
        {
            return false;
        }
        return std::equal(rootStr.begin(), rootStr.end(), fullStr.begin(), [](wchar_t lhs, wchar_t rhs) {
            return std::towlower(lhs) == std::towlower(rhs);
        });
    }
}
