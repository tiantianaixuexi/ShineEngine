#pragma once

#include <cstdint>
#include <string>

#include "loader/image/image_loader.h"
#include "shine_define.h"
#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine::gameplay::world {
class MapAsset;
}

namespace shine::manager {
class IRuntimeAsset;

enum class EAssetType : u8 {
    Image,
    Model,
    Map,
    Unknown
};

struct AssetHandle {
    uint64_t   id          = 0;
    uint32_t   generation  = 1;
    EAssetType type        = EAssetType::Unknown;
    SString    path        = {};
    bool       fromPackage = false;

    [[nodiscard]] bool isValid() const noexcept { return id != 0; }
};

struct TextureResourceHandle {
    uint64_t           id = 0;
    [[nodiscard]] bool isValid() const noexcept { return id != 0; }
};

class IAssetImportPipeline {
public:
    virtual ~IAssetImportPipeline()                                   = default;
    virtual AssetHandle LoadTextureAsset(const std::string &filePath) = 0;
    virtual AssetHandle LoadModel(const std::string &filePath)        = 0;
    virtual AssetHandle LoadMapAsset(const std::string &filePath)     = 0;
};

class ITextureBridge {
public:
    virtual ~ITextureBridge()                                                                           = default;
    virtual TextureResourceHandle  CreateTextureResource(const AssetHandle &imageAsset)                 = 0;
    virtual TextureResourceHandle  CreateTextureResourceByPath(STextView filePath)                      = 0;
    [[nodiscard]] virtual uint32_t GetTextureNativeId(const TextureResourceHandle &textureHandle) const = 0;
    virtual void                   ReleaseTextureResource(const TextureResourceHandle &textureHandle)   = 0;
};

class IAssetCatalog {
public:
    virtual ~IAssetCatalog()                                                              = default;
    [[nodiscard]] virtual IRuntimeAsset *GetRuntimeAsset(const AssetHandle &handle) const = 0;
    [[nodiscard]] virtual AssetHandle    GetAssetHandleByPath(STextView filePath) const   = 0;
    [[nodiscard]] virtual bool           IsAssetLoaded(const AssetHandle &handle) const   = 0;
};

class IWorldAssetBridge {
public:
    // 禁止拷贝和移动
    IWorldAssetBridge(const IWorldAssetBridge &)            = delete;
    IWorldAssetBridge &operator=(const IWorldAssetBridge &) = delete;
    IWorldAssetBridge(IWorldAssetBridge &&)                 = delete;
    IWorldAssetBridge &operator=(IWorldAssetBridge &&)      = delete;

    IWorldAssetBridge() = default;

    virtual ~IWorldAssetBridge()                                                                  = default;
    virtual AssetHandle                              LoadMapAsset(STextView filePath)             = 0;
    [[nodiscard]] virtual gameplay::world::MapAsset *GetMapAsset(const AssetHandle &handle) const = 0;
    virtual void                                     UnloadAsset(const AssetHandle &handle)       = 0;
};

class IImageAssetProvider {
public:
    // 禁止拷贝和移动
    IImageAssetProvider(const IImageAssetProvider &)            = delete;
    IImageAssetProvider &operator=(const IImageAssetProvider &) = delete;
    IImageAssetProvider(IImageAssetProvider &&)                 = delete;
    IImageAssetProvider &operator=(IImageAssetProvider &&)      = delete;

    IImageAssetProvider()                                                                       = default;
    virtual ~IImageAssetProvider()                                                              = default;
    [[nodiscard]] virtual loader::IImageLoader *GetImageLoader(const AssetHandle &handle) const = 0;
};
} // namespace shine::manager
