#include "util/EngineDirectoryService.h"

namespace shine::util
{
    bool EngineDirectoryService::Init(EngineContext& ctx)
    {
        (void)ctx;
        projectRootDirectory_ = std::filesystem::absolute(std::filesystem::current_path());
        if (!RegisterDirectory("Content", "Content"))
        {
            return false;
        }
        if (!RegisterDirectory("Config", "Config"))
        {
            return false;
        }
        RegisterDirectory("Saved", "Saved");
        RegisterDirectory("Logs", "Logs");
        contentDirectory_ = GetDirectory("Content");
        configDirectory_ = GetDirectory("Config");
        return true;
    }

    void EngineDirectoryService::Shutdown(EngineContext& ctx)
    {
        (void)ctx;
        directories_.clear();
        projectRootDirectory_.clear();
        contentDirectory_.clear();
        configDirectory_.clear();
    }

    bool EngineDirectoryService::RegisterDirectory(const shine::SString& key, const std::filesystem::path& directoryPath, bool createIfMissing)
    {
        if (key.empty())
        {
            return false;
        }

        std::filesystem::path resolvedPath = directoryPath;
        if (!resolvedPath.is_absolute())
        {
            resolvedPath = projectRootDirectory_ / resolvedPath;
        }

        std::error_code ec;
        if (createIfMissing)
        {
            std::filesystem::create_directories(resolvedPath, ec);
            if (ec)
            {
                return false;
            }
        }

        directories_[key] = std::filesystem::absolute(resolvedPath);
        return true;
    }

    std::filesystem::path EngineDirectoryService::GetDirectory(const shine::SString& key) const
    {
        if (const auto it = directories_.find(key); it != directories_.end())
        {
            return it->second;
        }
        return {};
    }

    bool EngineDirectoryService::HasDirectory(const shine::SString& key) const
    {
        return directories_.contains(key);
    }
}
