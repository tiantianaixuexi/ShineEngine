#pragma once

#include <memory>

#include "manager/AssetInterfaces.h"
#include "loader/model/model_loader.h"

namespace shine::manager
{
    class AssetCatalogImpl;

    class AssetImportPipelineImpl final : public IAssetImportPipeline
    {
    public:
        explicit AssetImportPipelineImpl(AssetCatalogImpl& catalog);

        AssetHandle LoadTextureAsset(const std::string& filePath) override;
        AssetHandle LoadModel(const std::string& filePath) override;
        AssetHandle LoadModel(const std::string& filePath, loader::IModelLoader::ProgressCallback progressCallback);
        AssetHandle LoadMapAsset(const std::string& filePath) override;
        AssetHandle LoadImageFromMemory(const void* data, size_t size, const std::string& formatHint);

    private:
        std::unique_ptr<loader::IImageLoader> CreateImageLoader(const std::string& format) const;
        std::unique_ptr<loader::IModelLoader> CreateModelLoader(const std::string& format) const;
        std::string DetectImageFormat(const void* data, size_t size) const;

    private:
        AssetCatalogImpl& catalog_;
    };
}
