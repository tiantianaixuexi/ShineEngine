#include "util/EngineDirectoryService.h"
#include "util/path_util.h"

#include "fmt/base.h"

#include <vector>

namespace shine::util
{
    namespace
    {
        std::optional<std::filesystem::path> FindProjectRootFrom(const std::filesystem::path& startPath)
        {
            if (startPath.empty())
            {
                return std::nullopt;
            }

            std::error_code ec;
            auto current = std::filesystem::absolute(startPath, ec);
            if (ec)
            {
                return std::nullopt;
            }

            for (int depth = 0; depth < 8; ++depth)
            {
                const auto contentPath = current / "Content";
                const auto configPath = current / "Config";
                if (std::filesystem::exists(contentPath, ec) && !ec &&
                    std::filesystem::exists(configPath, ec) && !ec)
                {
                    return current;
                }

                const auto parent = current.parent_path();
                if (parent.empty() || parent == current)
                {
                    break;
                }
                current = parent;
            }

            return std::nullopt;
        }

        std::filesystem::path ResolveProjectRootDirectory()
        {
            std::vector<std::filesystem::path> candidateRoots;
            candidateRoots.emplace_back(std::filesystem::current_path());
            if (const auto executableDirectory = get_executable_directory();
                executableDirectory.has_value() && !executableDirectory->empty())
            {
                candidateRoots.emplace_back(executableDirectory->c_str());
            }

            for (const auto& candidate : candidateRoots)
            {
                if (auto resolved = FindProjectRootFrom(candidate); resolved.has_value())
                {
                    return *resolved;
                }
            }

            return std::filesystem::absolute(std::filesystem::current_path());
        }
    }

    bool EngineDirectoryService::Init(EngineContext& ctx)
    {
        (void)ctx;
        projectRootDirectory_ = ResolveProjectRootDirectory();
        fmt::println("EngineDirectoryService: project root = {}", projectRootDirectory_.string());
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
        fmt::println("EngineDirectoryService: Content = {}", contentDirectory_.string());
        fmt::println("EngineDirectoryService: Config = {}", configDirectory_.string());
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
            resolvedPath = std::filesystem::path(join_path(STextView::from_string(projectRootDirectory_.string()), STextView::from_string(resolvedPath.string())).c_str());
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
