#pragma once

#include <cstdint>
#include <expected>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "EngineCore/asset/BaseAsset.h"
#include "EngineCore/asset/shared/AssetTypes.h"
#include "EngineCore/subsystem.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine::editor::asset
{
    struct AssetRegistryDependency
    {
        shine::AssetID assetID = shine::InvalidAssetID;
        bool hardReference = true;

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return assetID != shine::InvalidAssetID;
        }
    };

    struct AssetRegistryBuildState
    {
        std::uint32_t bundleId = 0;
        std::uint32_t version = 1;
        bool cooked = false;
    };

    struct AssetRegistryRecord
    {
        shine::AssetID assetID = shine::InvalidAssetID;

        shine::StringId nameId = shine::InvalidStringId;
        shine::StringId logicalPathId = shine::InvalidStringId;
        shine::StringId sourcePathId = shine::InvalidStringId;

        shine::EAssetKind kind = shine::EAssetKind::Unknown;

        std::uint32_t dependencyOffset = 0;
        std::uint32_t dependencyCount = 0;

        AssetRegistryBuildState buildState{};

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return assetID != shine::InvalidAssetID;
        }
    };

    class AssetRegistry final : public shine::Subsystem
    {
    public:
        AssetRegistry() = default;
        ~AssetRegistry() override = default;

        AssetRegistry(const AssetRegistry&) = delete;
        AssetRegistry& operator=(const AssetRegistry&) = delete;
        AssetRegistry(AssetRegistry&&) = delete;
        AssetRegistry& operator=(AssetRegistry&&) = delete;

        bool Init(EngineContext& ctx) override;
        void Shutdown(EngineContext& ctx) override;

        void Clear();

        void RegisterAsset(const std::shared_ptr<AssetBase>& asset);
        bool RemoveAsset(shine::AssetID assetID);

        [[nodiscard]] bool ContainsAsset(shine::AssetID assetID) const noexcept;
        [[nodiscard]] bool ContainsPath(shine::STextView logicalPath) const noexcept;

        [[nodiscard]] std::shared_ptr<AssetBase> GetAsset(shine::AssetID assetID) const;
        [[nodiscard]] std::shared_ptr<AssetBase> GetAssetByLogicalPath(shine::STextView logicalPath) const noexcept;

        [[nodiscard]] std::expected<AssetRegistryRecord, shine::SString> GetRecord(shine::AssetID assetID) const;
        [[nodiscard]] std::expected<AssetRegistryRecord, shine::SString> GetRecordByLogicalPath(shine::STextView logicalPath) const noexcept;

        [[nodiscard]] std::vector<std::shared_ptr<AssetBase>> GetAssetsByKind(shine::EAssetKind kind) const;
        [[nodiscard]] std::vector<AssetRegistryRecord> GetRecordsByKind(shine::EAssetKind kind) const noexcept;

        [[nodiscard]] const std::vector<AssetRegistryRecord>& GetAllRecords() const noexcept;
        [[nodiscard]] std::vector<std::shared_ptr<AssetBase>> GetAllAssets() const;

        bool AddDependency(
            shine::AssetID ownerAssetID,
            shine::AssetID dependencyAssetID,
            bool hardReference = true);

        bool RemoveDependency(
            shine::AssetID ownerAssetID,
            shine::AssetID dependencyAssetID);

        void ClearDependencies(shine::AssetID ownerAssetID);

        [[nodiscard]] std::vector<AssetRegistryDependency> GetDependencies(shine::AssetID assetID) const;
        [[nodiscard]] std::vector<shine::AssetID> GetReferencers(shine::AssetID assetID) const;
        [[nodiscard]] std::vector<shine::AssetID> BuildPreloadList(const std::vector<shine::AssetID>& roots) const;

        [[nodiscard]] std::expected<void, shine::SString> ValidateForCook(shine::AssetID assetID) const;

        void RegisterFactory(std::unique_ptr<shine::IAssetFactory> factory);
        [[nodiscard]] bool HasFactory(shine::EAssetKind kind) const noexcept;
        [[nodiscard]] std::shared_ptr<AssetBase> CreateAsset(const shine::AssetCreateContext& context);

        [[nodiscard]] shine::StringId InternString(shine::STextView text);
        [[nodiscard]] shine::StringId FindInternedString(shine::STextView text) const noexcept;
        [[nodiscard]] shine::STextView ResolveString(shine::StringId id) const noexcept;

        [[nodiscard]] std::size_t GetAssetCount() const noexcept;
        [[nodiscard]] std::size_t GetDependencyCount() const noexcept;

    private:
        struct IdToIndex
        {
            shine::AssetID id = shine::InvalidAssetID;
            shine::AssetHandle handle = shine::InvalidHandle;
        };

        [[nodiscard]] shine::StringId NormalizeAndInternLogicalPath(shine::STextView logicalPath);
        [[nodiscard]] shine::StringId FindLogicalPathId(shine::STextView logicalPath) const noexcept;
        [[nodiscard]] shine::StringId InternStringInternal(shine::STextView text);
        [[nodiscard]] shine::StringId FindInternedStringInternal(shine::STextView text) const noexcept;

        [[nodiscard]] shine::AssetHandle FindRecordHandle(shine::AssetID assetID) const noexcept;
        [[nodiscard]] AssetRegistryRecord* FindMutableRecord(shine::AssetID assetID) noexcept;
        [[nodiscard]] const AssetRegistryRecord* FindRecordPtr(shine::AssetID assetID) const noexcept;

        bool UpsertRecord(const AssetRegistryRecord& record);
        void RebuildRecordLookupTables();
        void RebuildDependencyPoolForAsset(
            shine::AssetID ownerAssetID,
            const std::vector<AssetRegistryDependency>& dependencies);

    private:
        std::unordered_map<shine::AssetID, std::shared_ptr<shine::editor::asset::AssetBase>> assetsById_;

        std::vector<AssetRegistryRecord> records_;
        std::vector<IdToIndex> idToIndexMap_;

        std::unordered_map<shine::StringId, shine::AssetHandle> handleByLogicalPath_;
        std::unordered_map<shine::EAssetKind, std::vector<shine::AssetHandle>> handlesByKind_;

        std::vector<AssetRegistryDependency> dependencyPool_;

        std::vector<shine::SString> stringPool_;
        std::unordered_map<std::string_view, shine::StringId> stringToId_;

        std::unordered_map<shine::EAssetKind, std::unique_ptr<shine::IAssetFactory>> factories_;
    };
}