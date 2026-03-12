#include "manager/runtime_asset.h"

#include "editor/asset/editor_runtime_asset_bridge.h"

namespace shine::manager {
void RegisterRuntimeAssetLoaders(shine::editor::asset::EditorRuntimeAssetBridge &bridge) {
    bridge.RegisterRuntimeLoader(EAssetKind::Texture,
                                 [](manager::IAssetImportPipeline &assetManager, const std::string &path) -> std::expected<manager::AssetHandle, std::string> {
                                     auto handle = assetManager.LoadTextureAsset({path});
                                     if (!handle.isValid()) {
                                         return std::unexpected("运行时纹理加载失败");
                                     }
                                     return handle;
                                 });

    bridge.RegisterRuntimeLoader(EAssetKind::Mesh,
                                 [](manager::IAssetImportPipeline &assetManager, const std::string &path) -> std::expected<manager::AssetHandle, std::string> {
                                     auto handle = assetManager.LoadModel({path});
                                     if (!handle.isValid()) {
                                         return std::unexpected("运行时模型加载失败");
                                     }
                                     return handle;
                                     {
                                     }
                                 });

    bridge.RegisterRuntimeLoader(EAssetKind::World,
                                 [](manager::IAssetImportPipeline &assetManager, const std::string &path) -> std::expected<manager::AssetHandle, std::string> {
                                     auto handle = assetManager.LoadMapAsset({path});
                                     if (!handle.isValid()) {
                                         return std::unexpected("运行时Map加载失败");
                                     }
                                     return handle;
                                 });
}
} // namespace shine::manager
