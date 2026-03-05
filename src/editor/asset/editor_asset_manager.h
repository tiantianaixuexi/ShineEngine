#pragma once

#include <memory>
#include <string>
#include <expected>
#include <unordered_map>
#include <vector>

#include "EngineCore/subsystem.h"
#include "EngineCore/asset/asset_base.h"

namespace shine::editor::asset
{
    using EditorAssetDependency = AssetDependencyRef;

    struct EditorAssetBuildState
    {
        uint32_t bundleId = 0;
        uint32_t version = 1;
        bool cooked = false;
    };

    struct EditorAssetRecord
    {
        algorithm::UUID assetID;
        std::string logicalPath;
        std::string sourcePath;
        std::string packagePath;
        EEditorAssetType type = EEditorAssetType::Unknown;
        std::vector<EditorAssetDependency> dependencies;
        EditorAssetBuildState buildState;
    };

    class EditorAssetManager : public shine::Subsystem
    {
    public:
        void RegisterAsset(std::shared_ptr<IAssetBase> asset);
        bool RemoveAsset(const algorithm::UUID& id);
        std::shared_ptr<IAssetBase> GetAsset(const algorithm::UUID& id) const;
        std::shared_ptr<IAssetBase> GetAssetByPath(const std::string& path) const;
        std::vector<std::shared_ptr<IAssetBase>> GetAllAssets() const;
        bool UpsertAssetRecord(EditorAssetRecord record);
        std::expected<EditorAssetRecord, std::string> GetAssetRecord(const algorithm::UUID& id) const;
        std::expected<EditorAssetRecord, std::string> GetAssetRecordByPath(const std::string& anyPath) const;
        std::vector<EditorAssetRecord> GetAllAssetRecords() const;
        bool AddDependency(const algorithm::UUID& owner, const algorithm::UUID& dependency, bool hardReference = true);
        std::vector<algorithm::UUID> BuildPreloadList(const std::vector<algorithm::UUID>& roots) const;
        std::expected<void, std::string> ValidateForCook(const algorithm::UUID& id) const;
        std::unordered_map<uint32_t, std::vector<algorithm::UUID>> BuildBundleIndex() const;
        std::vector<std::string> BuildCookManifest() const;

    private:
        std::unordered_map<algorithm::UUID, std::shared_ptr<IAssetBase>, algorithm::UUID::Hash> assetsById_;
        std::unordered_map<std::string, algorithm::UUID> idByPath_;
        std::unordered_map<algorithm::UUID, EditorAssetRecord, algorithm::UUID::Hash> recordsById_;
        std::unordered_map<std::string, algorithm::UUID> idByLogicalPath_;
        std::unordered_map<std::string, algorithm::UUID> idBySourcePath_;
        std::unordered_map<std::string, algorithm::UUID> idByPackagePath_;
    };
}
