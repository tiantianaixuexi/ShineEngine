#include "manager/AssetCatalogImpl.h"

#include "util/path_util.h"

namespace shine::manager
{
    bool AssetCatalogImpl::ResolveTypeById(uint64_t id, EAssetType& outType) const
    {
        auto it = runtimeAssetTypes_.find(id);
        if (it == runtimeAssetTypes_.end())
        {
            outType = EAssetType::Unknown;
            return false;
        }
        outType = it->second;
        return true;
    }

    AssetHandle AssetCatalogImpl::BuildHandle(uint64_t id, EAssetType type, const std::string& path) const
    {
        AssetHandle handle;
        handle.id = id;
        handle.type = type;
        handle.path = path;
        return handle;
    }

    AssetHandle AssetCatalogImpl::RegisterRuntimeAsset(EAssetType type, const std::string& logicalPath, std::unique_ptr<IRuntimeAsset> asset)
    {
        if (!asset)
        {
            return {};
        }
        const auto handle = BuildHandle(nextHandleId_++, type, logicalPath);
        runtimeAssets_[handle.id] = std::move(asset);
        runtimeAssetTypes_[handle.id] = type;
        if (!logicalPath.empty())
        {
            pathToHandle_[util::normalize_asset_path(logicalPath)] = handle.id;
        }
        return handle;
    }

    IRuntimeAsset* AssetCatalogImpl::GetRuntimeAsset(const AssetHandle& handle) const
    {
        if (!handle.isValid())
        {
            return nullptr;
        }
        auto it = runtimeAssets_.find(handle.id);
        if (it == runtimeAssets_.end())
        {
            return nullptr;
        }
        return it->second.get();
    }

    AssetHandle AssetCatalogImpl::GetAssetHandleByPath(const std::string& filePath) const
    {
        const auto normalizedPath = util::normalize_asset_path(filePath);
        auto it = pathToHandle_.find(normalizedPath);
        if (it == pathToHandle_.end())
        {
            return {};
        }
        EAssetType type = EAssetType::Unknown;
        ResolveTypeById(it->second, type);
        return BuildHandle(it->second, type, filePath);
    }

    bool AssetCatalogImpl::IsAssetLoaded(const AssetHandle& handle) const
    {
        if (!handle.isValid())
        {
            return false;
        }
        auto typeIt = runtimeAssetTypes_.find(handle.id);
        if (typeIt == runtimeAssetTypes_.end())
        {
            return false;
        }
        return typeIt->second == handle.type && runtimeAssets_.find(handle.id) != runtimeAssets_.end();
    }

    loader::IImageLoader* AssetCatalogImpl::GetImageLoader(const AssetHandle& handle) const
    {
        if (!handle.isValid() || handle.type != EAssetType::Image)
        {
            return nullptr;
        }
        auto* imageAsset = dynamic_cast<RuntimeImageAsset*>(GetRuntimeAsset(handle));
        return imageAsset ? imageAsset->getLoader() : nullptr;
    }

    loader::IModelLoader* AssetCatalogImpl::GetModelLoader(const AssetHandle& handle) const
    {
        if (!handle.isValid() || handle.type != EAssetType::Model)
        {
            return nullptr;
        }
        auto* modelAsset = dynamic_cast<RuntimeModelAsset*>(GetRuntimeAsset(handle));
        return modelAsset ? modelAsset->getLoader() : nullptr;
    }

    gameplay::world::MapAsset* AssetCatalogImpl::GetMapAsset(const AssetHandle& handle) const
    {
        if (!handle.isValid() || handle.type != EAssetType::Map)
        {
            return nullptr;
        }
        auto* mapAsset = dynamic_cast<RuntimeMapAsset*>(GetRuntimeAsset(handle));
        return mapAsset ? mapAsset->getMap() : nullptr;
    }

    void AssetCatalogImpl::UnloadAsset(const AssetHandle& handle)
    {
        if (!handle.isValid())
        {
            return;
        }
        runtimeAssets_.erase(handle.id);
        runtimeAssetTypes_.erase(handle.id);
        if (!handle.path.empty())
        {
            pathToHandle_.erase(util::normalize_asset_path(handle.path));
        }
    }

    void AssetCatalogImpl::UnloadAllAssets()
    {
        runtimeAssets_.clear();
        runtimeAssetTypes_.clear();
        pathToHandle_.clear();
    }

    uint64_t AssetCatalogImpl::PeekNextHandleId() const
    {
        return nextHandleId_;
    }
}
