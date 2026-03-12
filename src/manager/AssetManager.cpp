#include "AssetManager.h"

#include "image/Texture.h"
#include "manager/AssetCatalogImpl.h"
#include "manager/AssetImportPipelineImpl.h"
#include "manager/AssetTextureBridgeImpl.h"
#include "fmt/format.h"

namespace shine::manager
{
    AssetManager::AssetManager()
    {
        catalog_ = std::make_unique<AssetCatalogImpl>();
        importPipeline_ = std::make_unique<AssetImportPipelineImpl>(*catalog_);
        textureBridge_ = std::make_unique<AssetTextureBridgeImpl>(*catalog_, importPipeline_.get());
    }

    AssetManager::~AssetManager()
    {
        ShutdownEvent();
    }

    void AssetManager::Initialize()
    {
    }

    void AssetManager::ShutdownEvent()
    {
        UnloadAllAssets();
    }

    AssetHandle AssetManager::RegisterRuntimeAsset(EAssetType type, STextView  logicalPath, std::unique_ptr<IRuntimeAsset> asset)
    {
        return catalog_->RegisterRuntimeAsset(type, logicalPath, std::move(asset));
    }

    IRuntimeAsset* AssetManager::GetRuntimeAsset(const AssetHandle& handle) const
    {
        return catalog_->GetRuntimeAsset(handle);
    }

    AssetHandle AssetManager::LoadTextureAsset(STextView filePath)
    {
        return importPipeline_->LoadTextureAsset(filePath);
    }

    AssetHandle AssetManager::LoadImageFromMemory(const void* data, size_t size, const std::string& formatHint)
    {
        return importPipeline_->LoadImageFromMemory(data, size, formatHint);
    }

    loader::IImageLoader* AssetManager::GetImageLoader(const AssetHandle& handle) const
    {
        return catalog_->GetImageLoader(handle);
    }

    std::shared_ptr<image::STexture> AssetManager::LoadTexture(STextView  filePath)
    {
        auto assetHandle = LoadTextureAsset(filePath);
        if (!assetHandle.isValid())
        {
            return nullptr;
        }
        auto texture = std::make_shared<image::STexture>();
        if (!texture->InitializeFromAsset(assetHandle, *this))
        {
            return nullptr;
        }
        return texture;
    }

    AssetHandle AssetManager::LoadModel(STextView  filePath)
    {
        return importPipeline_->LoadModel(filePath);
    }

    AssetHandle AssetManager::LoadModel(STextView  filePath, loader::IModelLoader::ProgressCallback progressCallback)
    {
        return importPipeline_->LoadModel(filePath, std::move(progressCallback));
    }

    loader::IModelLoader* AssetManager::GetModelLoader(const AssetHandle& handle) const
    {
        return catalog_->GetModelLoader(handle);
    }

    std::expected<std::vector<loader::MeshData>, std::string> AssetManager::GetModelMeshes(const AssetHandle& handle) const
    {
        auto* loader = GetModelLoader(handle);
        if (!loader)
        {
            return std::unexpected("无效模型句柄或模型未加载");
        }
        if (!loader->isLoaded())
        {
            return std::unexpected("模型尚未加载完成");
        }
        auto meshes = loader->extractMeshData();
        if (meshes.empty())
        {
            return std::unexpected("模型未包含可渲染网格");
        }
        return meshes;
    }

    std::expected<loader::MeshData, std::string> AssetManager::GetModelMesh(const AssetHandle& handle, size_t meshIndex) const
    {
        auto meshesResult = GetModelMeshes(handle);
        if (!meshesResult.has_value())
        {
            return std::unexpected(meshesResult.error());
        }
        const auto& meshes = meshesResult.value();
        if (meshIndex >= meshes.size())
        {
            return std::unexpected(fmt::format("网格索引越界: {} / {}", meshIndex, meshes.size()));
        }
        return meshes[meshIndex];
    }

    std::expected<loader::MeshData, std::string> AssetManager::LoadModelMesh(STextView  filePath, size_t meshIndex, loader::IModelLoader::ProgressCallback progressCallback)
    {
        auto handle = LoadModel(filePath, std::move(progressCallback));
        if (!handle.isValid())
        {
            return std::unexpected(fmt::format("模型加载失败: {}", filePath));
        }
        return GetModelMesh(handle, meshIndex);
    }

    AssetHandle AssetManager::LoadMapAsset(STextView  filePath)
    {
        return importPipeline_->LoadMapAsset(filePath);
    }

    gameplay::world::MapAsset* AssetManager::GetMapAsset(const AssetHandle& handle) const
    {
        return catalog_->GetMapAsset(handle);
    }

    void AssetManager::UnloadAsset(const AssetHandle& handle)
    {
        catalog_->UnloadAsset(handle);
    }

    void AssetManager::UnloadAllAssets()
    {
        if (textureBridge_)
        {
            textureBridge_->ReleaseAll();
        }
        if (catalog_)
        {
            catalog_->UnloadAllAssets();
        }
    }

    bool AssetManager::IsAssetLoaded(const AssetHandle& handle) const
    {
        return catalog_->IsAssetLoaded(handle);
    }

    AssetHandle AssetManager::GetAssetHandleByPath(STextView  filePath) const
    {
        return catalog_->GetAssetHandleByPath(filePath);
    }

    std::vector<std::string> AssetManager::GetSupportedImageFormats()
    {
        return {"png", "jpeg", "jpg"};
    }

    std::vector<std::string> AssetManager::GetSupportedModelFormats()
    {
        return {"gltf", "glb", "obj"};
    }

    TextureResourceHandle AssetManager::CreateTextureResource(const AssetHandle& imageAsset)
    {
        return textureBridge_->CreateTextureResource(imageAsset);
    }

    TextureResourceHandle AssetManager::CreateTextureResourceByPath(STextView filePath)
    {
        return textureBridge_->CreateTextureResourceByPath(filePath);
    }

    uint32_t AssetManager::GetTextureNativeId(const TextureResourceHandle& textureHandle) const
    {
        return textureBridge_->GetTextureNativeId(textureHandle);
    }

    void AssetManager::ReleaseTextureResource(const TextureResourceHandle& textureHandle)
    {
        textureBridge_->ReleaseTextureResource(textureHandle);
    }
}
