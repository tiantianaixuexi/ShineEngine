#pragma once

#include <unordered_map>

#include "manager/AssetInterfaces.h"

namespace shine::manager
{
    class AssetCatalogImpl;

    class AssetTextureBridgeImpl final : public ITextureBridge
    {
    public:
        explicit AssetTextureBridgeImpl(AssetCatalogImpl& catalog, IAssetImportPipeline* importPipeline);

        void SetImportPipeline(IAssetImportPipeline* importPipeline);
        TextureResourceHandle CreateTextureResource(const AssetHandle& imageAsset) override;
        TextureResourceHandle CreateTextureResourceByPath(STextView filePath) override;
        uint32_t GetTextureNativeId(const TextureResourceHandle& textureHandle) const override;
        void ReleaseTextureResource(const TextureResourceHandle& textureHandle) override;
        void ReleaseAll();

    private:
        AssetCatalogImpl& catalog_;
        IAssetImportPipeline* importPipeline_ = nullptr;
        std::unordered_map<uint64_t, uint64_t> textureResourceToRenderHandle_;
        std::unordered_map<uint64_t, uint64_t> renderHandleToTextureResource_;
        uint64_t nextTextureResourceId_ = 1;
    };
}
