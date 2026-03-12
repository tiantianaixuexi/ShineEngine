#include "editor_runtime_asset_bridge.h"

#include "EngineCore/engine_context.h"
#include "gameplay/world/world_service.h"

namespace shine::editor::asset
{
    EditorRuntimeAssetBridge::EditorRuntimeAssetBridge(EditorAssetManager* editorAssetManager)
        : editorAssetManager_(editorAssetManager)
    {
    }

    bool EditorRuntimeAssetBridge::Init(EngineContext& ctx)
    {
        if (!editorAssetManager_)
        {
            editorAssetManager_ = ctx.GetSystem<EditorAssetManager>();
        }
        auto* runtimeAssetManager = ctx.GetSystem<manager::AssetManager>();
        if (runtimeAssetManager)
        {
            runtimeImportPipeline_ = runtimeAssetManager;
        }
        worldService_ = ctx.GetSystem<gameplay::world::WorldService>();
        manager::RegisterRuntimeAssetLoaders(*this);
        return editorAssetManager_ && runtimeImportPipeline_;
    }

    void EditorRuntimeAssetBridge::RegisterRuntimeLoader(EAssetKind type, RuntimeLoader loader)
    {
        if (!loader)
        {
            return;
        }
        runtimeLoaders_[type] = std::move(loader);
    }

    std::expected<std::string, std::string> EditorRuntimeAssetBridge::ResolveRuntimePath(const EditorAssetRecord& record) const
    {
        //if (!record.packagePath.empty())
        //{
        //    return record.packagePath;
        //}
        //if (!record.sourcePath.empty())
        //{
        //    return record.sourcePath;
        //}
        //if (!record.logicalPath.empty())
        //{
        //    return record.logicalPath;
        //}
        return std::unexpected("资产路径为空");
    }

    std::expected<manager::AssetHandle, std::string> EditorRuntimeAssetBridge::LoadRuntimeAssetByEditorId(const AssetID &id)
    {
        if (!editorAssetManager_ || !runtimeImportPipeline_)
        {
            return std::unexpected("资产子系统未初始化");
        }

        auto recordResult = editorAssetManager_->GetAssetRecord(id);
        if (!recordResult.has_value())
        {
            return std::unexpected(recordResult.error());
        }

        const auto& record = recordResult.value();
        auto pathResult = ResolveRuntimePath(record);
        if (!pathResult.has_value())
        {
            return std::unexpected(pathResult.error());
        }
        const auto& path = pathResult.value();

        auto loaderIt = runtimeLoaders_.find(record.kind);
        if (loaderIt == runtimeLoaders_.end())
        {
            return std::unexpected("未注册该编辑器资产类型的运行时加载器");
        }
        auto loadResult = loaderIt->second(*runtimeImportPipeline_, path);
        if (!loadResult.has_value())
        {
            return std::unexpected(loadResult.error());
        }
        return loadResult.value();
    }

    RuntimePreloadResult EditorRuntimeAssetBridge::PreloadRuntimeAssets(const std::vector<AssetID> &rootAssets)
    {
        RuntimePreloadResult result;
        if (!editorAssetManager_)
        {
            result.errors.push_back("EditorAssetManager 未初始化");
            return result;
        }

        const auto preloadList = editorAssetManager_->BuildPreloadList(rootAssets);
        result.loadedHandles.reserve(preloadList.size());
        for (const auto& id : preloadList)
        {
            auto loadResult = LoadRuntimeAssetByEditorId(id);
            if (loadResult.has_value())
            {
                result.loadedHandles.push_back(loadResult.value());
            }
            else
            {
                result.failedAssets.push_back(id);
                result.errors.push_back(loadResult.error());
            }
        }
        return result;
    }

    RuntimePreloadResult EditorRuntimeAssetBridge::PreloadBundleRuntimeAssets(uint32_t bundleId)
    {
        RuntimePreloadResult result;
        if (!editorAssetManager_)
        {
            result.errors.emplace_back("EditorAssetManager 未初始化");
            return result;
        }
        if (bundleId == 0)
        {
            result.errors.emplace_back("bundleId 无效");
            return result;
        }

        //const auto bundleIndex = editorAssetManager_->BuildBundleIndex();
        //auto it = bundleIndex.find(bundleId);
        //if (it == bundleIndex.end())
        //{
        //    result.errors.push_back("未找到指定Bundle");
        //    return result;
        //}
        //return PreloadRuntimeAssets(it->second);

        return result;
    }

    std::expected<manager::AssetHandle, std::string> EditorRuntimeAssetBridge::ActivateWorldMapByEditorId(const AssetID &mapAssetId)
    {
        auto loadResult = LoadRuntimeAssetByEditorId(mapAssetId);
        if (!loadResult.has_value())
        {
            return std::unexpected(loadResult.error());
        }
        const auto mapHandle = loadResult.value();
        if (mapHandle.type != manager::EAssetType::Map)
        {
            return std::unexpected("目标资产不是Map类型");
        }

        if (!worldService_)
        {
            return std::unexpected("WorldService 未初始化");
        }
        if (!worldService_->activateMapAsset(mapHandle))
        {
            return std::unexpected("激活World Map失败");
        }
        return mapHandle;
    }
}
