#pragma once

#include "Algorithm/uuid.h"

#include <expected>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "EngineCore/subsystem.h"
#include "Manager/editor_asset_manager.h"
#include "manager/AssetManager.h"

namespace shine::gameplay::world
{
    class WorldService;
}

namespace shine::editor::asset
{
    struct RuntimePreloadResult
    {
        std::vector<manager::AssetHandle> loadedHandles;
        std::vector<AssetID>              failedAssets;
        std::vector<std::string> errors;
    };

    class EditorRuntimeAssetBridge : public shine::Subsystem
    {
    public:
        using RuntimeLoader = std::function<std::expected<manager::AssetHandle, std::string>(manager::IAssetImportPipeline&, const std::string&)>;

        explicit EditorRuntimeAssetBridge(EditorAssetManager* editorAssetManager = nullptr);

        bool Init(EngineContext& ctx) override;
        std::expected<manager::AssetHandle, std::string> LoadRuntimeAssetByEditorId(const AssetID &id);
        RuntimePreloadResult                             PreloadRuntimeAssets(const std::vector<AssetID> &rootAssets);
        RuntimePreloadResult PreloadBundleRuntimeAssets(uint32_t bundleId);
        std::expected<manager::AssetHandle, std::string> ActivateWorldMapByEditorId(const AssetID &mapAssetId);
        void RegisterRuntimeLoader(EAssetKind type, RuntimeLoader loader);

    private:
        std::expected<std::string, std::string> ResolveRuntimePath(const EditorAssetRecord& record) const;

    private:
        std::unordered_map<EAssetKind, RuntimeLoader> runtimeLoaders_;
        EditorAssetManager* editorAssetManager_ = nullptr;
        manager::IAssetImportPipeline* runtimeImportPipeline_ = nullptr;
        gameplay::world::WorldService* worldService_ = nullptr;
    };
}
