#include "manager/AssetTextureBridgeImpl.h"

#include "EngineCore/engine_context.h"
#include "manager/AssetCatalogImpl.h"
#include "render/resources/TextureManager.h"

namespace shine::manager
{
    AssetTextureBridgeImpl::AssetTextureBridgeImpl(AssetCatalogImpl& catalog, IAssetImportPipeline* importPipeline)
        : catalog_(catalog), importPipeline_(importPipeline)
    {
    }

    void AssetTextureBridgeImpl::SetImportPipeline(IAssetImportPipeline* importPipeline)
    {
        importPipeline_ = importPipeline;
    }

    TextureResourceHandle AssetTextureBridgeImpl::CreateTextureResource(const AssetHandle& imageAsset)
    {
        if (!imageAsset.isValid() || imageAsset.type != EAssetType::Image)
        {
            return {};
        }
        auto* textureManager = EngineContext::Get().GetSystem<render::TextureManager>();
        if (!textureManager)
        {
            return {};
        }
        auto* imageLoader = catalog_.GetImageLoader(imageAsset);
        if (!imageLoader || !imageLoader->isDecoded() || imageLoader->getImageData().empty())
        {
            return {};
        }

        render::TextureCreateInfo info;
        info.width = static_cast<int>(imageLoader->getWidth());
        info.height = static_cast<int>(imageLoader->getHeight());
        info.data = imageLoader->getImageData().data();
        info.generateMipmaps = false;
        info.linearFilter = true;
        info.clampToEdge = true;

        auto renderHandle = textureManager->CreateTexture(info);
        if (!renderHandle.isValid())
        {
            return {};
        }

        TextureResourceHandle textureHandle;
        textureHandle.id = nextTextureResourceId_++;
        textureResourceToRenderHandle_[textureHandle.id] = renderHandle.id;
        renderHandleToTextureResource_[renderHandle.id] = textureHandle.id;
        return textureHandle;
    }

    TextureResourceHandle AssetTextureBridgeImpl::CreateTextureResourceByPath(STextView filePath)
    {
        if (!importPipeline_)
        {
            return {};
        }
        auto imageAsset = importPipeline_->LoadTextureAsset(filePath);
        if (!imageAsset.isValid())
        {
            return {};
        }
        return CreateTextureResource(imageAsset);
    }

    uint32_t AssetTextureBridgeImpl::GetTextureNativeId(const TextureResourceHandle& textureHandle) const
    {
        if (!textureHandle.isValid())
        {
            return 0;
        }
        auto* textureManager = EngineContext::Get().GetSystem<render::TextureManager>();
        if (!textureManager)
        {
            return 0;
        }
        auto it = textureResourceToRenderHandle_.find(textureHandle.id);
        if (it == textureResourceToRenderHandle_.end())
        {
            return 0;
        }
        return textureManager->GetTextureId(render::TextureHandle{ .id = it->second });
    }

    void AssetTextureBridgeImpl::ReleaseTextureResource(const TextureResourceHandle& textureHandle)
    {
        if (!textureHandle.isValid())
        {
            return;
        }
        auto* textureManager = EngineContext::Get().GetSystem<render::TextureManager>();
        if (!textureManager)
        {
            return;
        }
        auto it = textureResourceToRenderHandle_.find(textureHandle.id);
        if (it == textureResourceToRenderHandle_.end())
        {
            return;
        }
        const auto renderId = it->second;
        textureManager->ReleaseTexture(render::TextureHandle{ .id = renderId });
        textureResourceToRenderHandle_.erase(it);
        renderHandleToTextureResource_.erase(renderId);
    }

    void AssetTextureBridgeImpl::ReleaseAll()
    {
        auto* textureManager = EngineContext::Get().GetSystem<render::TextureManager>();
        if (!textureManager)
        {
            textureResourceToRenderHandle_.clear();
            renderHandleToTextureResource_.clear();
            return;
        }
        for (const auto& [_, renderId] : textureResourceToRenderHandle_)
        {
            textureManager->ReleaseTexture(render::TextureHandle{ .id = renderId });
        }
        textureResourceToRenderHandle_.clear();
        renderHandleToTextureResource_.clear();
    }
}
