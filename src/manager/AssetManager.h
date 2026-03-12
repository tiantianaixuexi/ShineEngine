#pragma once

#include "Engine/Macro/RuntimeEditorSplit.h"
#include "EngineCore/subsystem.h"
#include "loader/core/loader.h"
#include "loader/model/model_loader.h"
#include "manager/AssetInterfaces.h"
#include "manager/runtime_asset.h"
#include <cstdint>
#include <expected>
#include <memory>
#include <string>
#include <vector>

// Windows.h 定义了 LoadImage 宏，已通过重命名函数避免冲突

// 前向声明
namespace shine::image {
class STexture;
}

namespace shine::manager {
class AssetCatalogImpl;
class AssetImportPipelineImpl;
class AssetTextureBridgeImpl;

class AssetManager : public shine::Subsystem, public IAssetImportPipeline, public ITextureBridge, public IAssetCatalog, public IWorldAssetBridge, public IImageAssetProvider {
public:
    AssetManager();
    ~AssetManager();

    /**
     * @brief 初始化资源管理器
     */
    void Initialize();

    /**
     * @brief 关闭资源管理器，释放所有资源
     */
    void ShutdownEvent();

    // ========================================================================
    // 图片资源管理
    // ========================================================================

    /**
     * @brief 加载图片资源（自动识别格式）
     * @param filePath 图片文件路径
     * @return 资源句柄，失败返回无效句柄
     */
    AssetHandle LoadTextureAsset(STextView filePath) override;

    /**
     * @brief 从内存加载图片资源
     * @param data 图片数据
     * @param size 数据大小
     * @param formatHint 格式提示（可选）
     * @return 资源句柄，失败返回无效句柄
     */
    AssetHandle LoadImageFromMemory(const void *data, size_t size, const std::string &formatHint = "");

    /**
     * @brief 获取图片加载器
     * @param handle 资源句柄
     * @return 图片加载器指针，失败返回nullptr
     */
    loader::IImageLoader *GetImageLoader(const AssetHandle &handle) const override;

    /**
     * @brief 加载图片并创建 STexture 资源（便利方法，类似 UE5 的 LoadObject）
     * 一步完成：加载图片 -> 创建 STexture -> 返回共享指针
     * @param filePath 图片文件路径
     * @return STexture 共享指针，失败返回空指针
     */
    std::shared_ptr<image::STexture> LoadTexture(STextView filePath);

    // ========================================================================
    // 模型资源管理
    // ========================================================================

    /**
     * @brief 加载模型资源（自动识别格式）
     * @param filePath 模型文件路径
     * @return 资源句柄，失败返回无效句柄
     */
    AssetHandle LoadModel(STextView filePath) override;
    AssetHandle LoadModel(STextView filePath, loader::IModelLoader::ProgressCallback progressCallback);

    /**
     * @brief 获取模型加载器
     * @param handle 资源句柄
     * @return 模型加载器指针，失败返回nullptr
     */
    loader::IModelLoader *GetModelLoader(const AssetHandle &handle) const;

    std::expected<std::vector<loader::MeshData>, std::string> GetModelMeshes(const AssetHandle &handle) const;
    std::expected<loader::MeshData, std::string>              GetModelMesh(const AssetHandle &handle, size_t meshIndex = 0) const;
    std::expected<loader::MeshData, std::string>              LoadModelMesh(STextView  filePath, size_t meshIndex = 0, loader::IModelLoader::ProgressCallback progressCallback = nullptr);

    AssetHandle                LoadMapAsset(STextView filePath) override;
    gameplay::world::MapAsset *GetMapAsset(const AssetHandle &handle) const override;
    IRuntimeAsset             *GetRuntimeAsset(const AssetHandle &handle) const override;
    AssetHandle                RegisterRuntimeAsset(EAssetType type, STextView logicalPath, std::unique_ptr<IRuntimeAsset> asset);
    TextureResourceHandle      CreateTextureResource(const AssetHandle &imageAsset) override;
    TextureResourceHandle      CreateTextureResourceByPath(STextView filePath) override;
    uint32_t                   GetTextureNativeId(const TextureResourceHandle &textureHandle) const override;
    void                       ReleaseTextureResource(const TextureResourceHandle &textureHandle) override;

    // ========================================================================
    // 通用资源管理
    // ========================================================================

    /**
     * @brief 卸载资源
     * @param handle 资源句柄
     */
    void UnloadAsset(const AssetHandle &handle) override;

    /**
     * @brief 卸载所有资源
     */
    void UnloadAllAssets();

    /**
     * @brief 检查资源是否存在
     * @param handle 资源句柄
     * @return true如果资源存在
     */
    bool IsAssetLoaded(const AssetHandle &handle) const override;

    /**
     * @brief 根据文件路径获取资源句柄
     * @param filePath 文件路径
     * @return 资源句柄，如果不存在返回无效句柄
     */
    AssetHandle GetAssetHandleByPath(STextView filePathh) const override;

    /**
     * @brief 获取支持的图片格式列表
     */
    static std::vector<std::string> GetSupportedImageFormats();

    static std::vector<std::string> GetSupportedModelFormats();

private:
    std::unique_ptr<AssetCatalogImpl>        catalog_;
    std::unique_ptr<AssetImportPipelineImpl> importPipeline_;
    std::unique_ptr<AssetTextureBridgeImpl>  textureBridge_;

private:
    AssetManager(const AssetManager &)            = delete;
    AssetManager &operator=(const AssetManager &) = delete;
    AssetManager(AssetManager &&)                 = delete;
    AssetManager &operator=(AssetManager &&)      = delete;
};
} // namespace shine::manager
