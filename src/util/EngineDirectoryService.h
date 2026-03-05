#pragma once

#include <filesystem>
#include <unordered_map>

#include "EngineCore/subsystem.h"
#include "string/shine_string.h"

namespace shine::util
{
    class EngineDirectoryService : public shine::Subsystem
    {
    public:
        bool Init(EngineContext& ctx) override;
        void Shutdown(EngineContext& ctx) override;

        bool RegisterDirectory(const shine::SString& key, const std::filesystem::path& directoryPath, bool createIfMissing = true);
        std::filesystem::path GetDirectory(const shine::SString& key) const;
        bool HasDirectory(const shine::SString& key) const;

        const std::filesystem::path& GetProjectRootDirectory() const { return projectRootDirectory_; }
        const std::filesystem::path& GetContentDirectory() const { return contentDirectory_; }
        const std::filesystem::path& GetConfigDirectory() const { return configDirectory_; }

    private:
        std::filesystem::path projectRootDirectory_;
        std::filesystem::path contentDirectory_;
        std::filesystem::path configDirectory_;
        std::unordered_map<shine::SString, std::filesystem::path> directories_;
    };
}
