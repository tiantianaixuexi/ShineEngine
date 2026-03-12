#pragma once

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "EngineCore/asset/shared/AssetTypes.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine::editor::asset
{
    struct AssetDependencyRef
    {
        shine::AssetID assetID = shine::InvalidAssetID;
        shine::StringId pathHintId = shine::InvalidStringId;
        bool hardReference = true;

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return assetID != shine::InvalidAssetID;
        }
    };

    struct AssetImportSettings
    {
        std::vector<std::pair<shine::StringId, shine::StringId>> options;
        bool generateMips = true;
        bool srgb = true;
        bool allowCompression = true;
        std::uint32_t version = 1;
    };

    struct AssetCookProfile
    {
        std::uint64_t sourceSizeBytes = 0;
        std::uint64_t cookedSizeBytes = 0;
        std::uint64_t lastCookTimestamp = 0;
        std::uint32_t bundleId = 0;
        bool streamable = false;
        bool compressed = false;
        shine::SString targetPlatform;
    };

    class AssetBase
    {
    public:
        AssetBase() = default;
        virtual ~AssetBase() = default;

        AssetBase(const AssetBase&) = default;
        AssetBase& operator=(const AssetBase&) = default;
        AssetBase(AssetBase&&) noexcept = default;
        AssetBase& operator=(AssetBase&&) noexcept = default;

        void Init(
            shine::AssetID id,
            shine::SString name,
            const shine::SString& logicalPath,
            shine::EAssetKind kind = shine::EAssetKind::Unknown);

        void SetID(shine::AssetID id) noexcept;
        void SetName(shine::SString name);
        void SetLogicalPath(const shine::SString& logicalPath);
        void SetSourcePath(const shine::SString& sourcePath);
        void SetSourceHash(shine::SString hash);
        void SetLifecycle(shine::EAssetLifecycle lifecycle) noexcept;

        void SetImportSettings(AssetImportSettings settings);
        void SetImportOption(shine::StringId key, shine::StringId value);
        [[nodiscard]] std::optional<shine::StringId> GetImportOption(shine::StringId key) const;
        void RemoveImportOption(shine::StringId key);
        void ClearImportOptions();

        void SetCookProfile(AssetCookProfile profile);
        void AddDependency(
            shine::AssetID dependencyID,
            shine::StringId pathHintId = shine::InvalidStringId,
            bool hardReference = true);
        [[nodiscard]] bool RemoveDependency(shine::AssetID dependencyID);
        void ClearDependencies();

        void MarkDirty() noexcept;
        void MarkClean() noexcept;
        void Touch() noexcept;
        void BumpVersion();


        [[nodiscard]] shine::AssetID GetID() const noexcept;
        [[nodiscard]] shine::STextView GetName() const noexcept;
        [[nodiscard]] shine::STextView GetLogicalPath() const noexcept;
        [[nodiscard]] shine::STextView GetSourcePath() const noexcept;
        [[nodiscard]] shine::STextView GetSourceHash() const noexcept;
        [[nodiscard]] shine::EAssetKind GetKind() const noexcept;
        [[nodiscard]] shine::EAssetLifecycle GetLifecycle() const noexcept;
        [[nodiscard]] std::uint32_t GetVersion() const noexcept;
        [[nodiscard]] std::uint64_t GetCreateTimestamp() const noexcept;
        [[nodiscard]] std::uint64_t GetModifiedTimestamp() const noexcept;
        [[nodiscard]] const AssetImportSettings& GetImportSettings() const noexcept;
        [[nodiscard]] const AssetCookProfile& GetCookProfile() const noexcept;
        [[nodiscard]] const std::vector<AssetDependencyRef>& GetDependencies() const noexcept;
        [[nodiscard]] bool IsDirty() const noexcept;
        [[nodiscard]] bool HasValidID() const noexcept;

    protected:
        [[nodiscard]] static std::uint64_t CurrentTimestamp() noexcept;

    private:
        shine::AssetID assetID_ = shine::InvalidAssetID;
        shine::SString assetName_;
        shine::SString logicalPath_;
        shine::SString sourcePath_;
        shine::SString sourceHash_;
        shine::EAssetKind assetKind_ = shine::EAssetKind::Unknown;
        shine::EAssetLifecycle lifecycle_ = shine::EAssetLifecycle::Imported;
        AssetImportSettings importSettings_;
        AssetCookProfile cookProfile_;
        std::vector<AssetDependencyRef> dependencies_;
        std::uint32_t assetVersion_ = 1;
        std::uint64_t createTimestamp_ = 0;
        std::uint64_t modifiedTimestamp_ = 0;
        bool dirty_ = false;
    };

    using IAssetBase = AssetBase;
}