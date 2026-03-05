#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "manager/AssetInterfaces.h"
#include "manager/runtime_asset.h"

namespace shine::manager
{
    class AssetCatalogImpl : public IAssetCatalog
    {
    public:
        AssetHandle RegisterRuntimeAsset(EAssetType type, const std::string& logicalPath, std::unique_ptr<IRuntimeAsset> asset);
        IRuntimeAsset* GetRuntimeAsset(const AssetHandle& handle) const override;
        AssetHandle GetAssetHandleByPath(const std::string& filePath) const override;
        bool IsAssetLoaded(const AssetHandle& handle) const override;
        loader::IImageLoader* GetImageLoader(const AssetHandle& handle) const;
        loader::IModelLoader* GetModelLoader(const AssetHandle& handle) const;
        gameplay::world::MapAsset* GetMapAsset(const AssetHandle& handle) const;
        void UnloadAsset(const AssetHandle& handle);
        void UnloadAllAssets();
        uint64_t PeekNextHandleId() const;

    private:
        bool ResolveTypeById(uint64_t id, EAssetType& outType) const;
        AssetHandle BuildHandle(uint64_t id, EAssetType type, const std::string& path) const;

    private:
        std::unordered_map<uint64_t, std::unique_ptr<IRuntimeAsset>> runtimeAssets_;
        std::unordered_map<uint64_t, EAssetType> runtimeAssetTypes_;
        std::unordered_map<std::string, uint64_t> pathToHandle_;
        uint64_t nextHandleId_ = 1;
    };
}
