#pragma once

#include <cstdint>
#include <string>

#include "loader/image/image_loader.h"

namespace shine::gameplay::world
{
    class MapAsset;
}

namespace shine::manager
{
    class IRuntimeAsset;

    enum class EAssetType
    {
        Image,
        Model,
        Map,
        Unknown
    };

    struct AssetHandle
    {
        uint64_t id = 0;
        uint32_t generation = 1;
        EAssetType type = EAssetType::Unknown;
        std::string path;
        bool fromPackage = false;

        bool isValid() const { return id != 0; }
    };

    struct TextureResourceHandle
    {
        uint64_t id = 0;
        bool isValid() const { return id != 0; }
    };

    class IAssetImportPipeline
    {
    public:
        virtual ~IAssetImportPipeline() = default;
        virtual AssetHandle LoadTextureAsset(const std::string& filePath) = 0;
        virtual AssetHandle LoadModel(const std::string& filePath) = 0;
        virtual AssetHandle LoadMapAsset(const std::string& filePath) = 0;
    };

    class ITextureBridge
    {
    public:
        virtual ~ITextureBridge() = default;
        virtual TextureResourceHandle CreateTextureResource(const AssetHandle& imageAsset) = 0;
        virtual TextureResourceHandle CreateTextureResourceByPath(const std::string& filePath) = 0;
        virtual uint32_t GetTextureNativeId(const TextureResourceHandle& textureHandle) const = 0;
        virtual void ReleaseTextureResource(const TextureResourceHandle& textureHandle) = 0;
    };

    class IAssetCatalog
    {
    public:
        virtual ~IAssetCatalog() = default;
        virtual IRuntimeAsset* GetRuntimeAsset(const AssetHandle& handle) const = 0;
        virtual AssetHandle GetAssetHandleByPath(const std::string& filePath) const = 0;
        virtual bool IsAssetLoaded(const AssetHandle& handle) const = 0;
    };

    class IWorldAssetBridge
    {
    public:
        virtual ~IWorldAssetBridge() = default;
        virtual AssetHandle LoadMapAsset(const std::string& filePath) = 0;
        virtual gameplay::world::MapAsset* GetMapAsset(const AssetHandle& handle) const = 0;
        virtual void UnloadAsset(const AssetHandle& handle) = 0;
    };

    class IImageAssetProvider
    {
    public:
        virtual ~IImageAssetProvider() = default;
        virtual loader::IImageLoader* GetImageLoader(const AssetHandle& handle) const = 0;
    };
}
