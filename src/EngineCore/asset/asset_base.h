#pragma once

#include "util/Algorithm/uuid.h"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace shine::editor::asset
{
    enum class EEditorAssetType
    {
        Texture,
        StaticMesh,
        Material,
        Shader,
        Scene,
        Audio,
        Script,
        Unknown
    };

    enum class EAssetLifecycle
    {
        Imported,
        Dirty,
        Cooking,
        Cooked,
        Failed
    };

    struct AssetDependencyRef
    {
        algorithm::UUID assetID;
        std::string pathHint;
        bool hardReference = true;
    };

    struct AssetImportSettings
    {
        std::unordered_map<std::string, std::string> options;
        bool generateMips = true;
        bool srgb = true;
        bool allowCompression = true;
        uint32_t version = 1;
    };

    struct AssetCookProfile
    {
        uint64_t sourceSizeBytes = 0;
        uint64_t cookedSizeBytes = 0;
        uint64_t lastCookTimestamp = 0;
        uint32_t bundleId = 0;
        bool streamable = false;
        bool compressed = false;
        std::string targetPlatform;
    };

    class IAssetBase
    {
    public:
        IAssetBase() = default;
        virtual ~IAssetBase() = default;

        void Init(std::string name, std::string path, EEditorAssetType type = EEditorAssetType::Unknown);
        void SetName(std::string name);
        void SetPath(std::string path);
        void SetSourceHash(std::string md5);
        void SetLifecycle(EAssetLifecycle lifecycle);
        void SetImportOption(std::string key, std::string value);
        std::optional<std::string> GetImportOption(const std::string& key) const;
        void SetCookProfile(AssetCookProfile profile);
        void AddDependency(const algorithm::UUID& dependencyID, std::string pathHint = {}, bool hardReference = true);
        bool RemoveDependency(const algorithm::UUID& dependencyID);
        void ClearDependencies();
        void Touch();
        void BumpVersion();

        const std::string& GetName() const noexcept;
        const std::string& GetPath() const noexcept;
        const std::string& GetSourceHash() const noexcept;
        const algorithm::UUID& GetID() const noexcept;
        EEditorAssetType GetType() const noexcept;
        EAssetLifecycle GetLifecycle() const noexcept;
        uint32_t GetVersion() const noexcept;
        uint64_t GetCreateTimestamp() const noexcept;
        uint64_t GetModifiedTimestamp() const noexcept;
        const AssetImportSettings& GetImportSettings() const noexcept;
        const AssetCookProfile& GetCookProfile() const noexcept;
        const std::vector<AssetDependencyRef>& GetDependencies() const noexcept;
        bool IsDirty() const noexcept;

    private:
        static uint64_t CurrentTimestamp();

    private:
        std::string assetName;
        std::string assetPath;
        std::string assetMd5;
        algorithm::UUID assetID;
        EEditorAssetType assetType = EEditorAssetType::Unknown;
        EAssetLifecycle lifecycle = EAssetLifecycle::Imported;
        AssetImportSettings importSettings;
        AssetCookProfile cookProfile;
        std::vector<AssetDependencyRef> dependencies;
        uint32_t assetVersion = 1;
        uint64_t createTimestamp = 0;
        uint64_t modifiedTimestamp = 0;
        bool dirty = false;
    };

}
