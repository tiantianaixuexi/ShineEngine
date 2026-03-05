#include "editor_asset_manager.h"

#include <algorithm>
#include <queue>
#include <unordered_set>

#include "fmt/format.h"
#include "util/path_util.h"

namespace shine::editor::asset
{
    void EditorAssetManager::RegisterAsset(std::shared_ptr<IAssetBase> asset)
    {
        if (!asset)
        {
            return;
        }
        const auto id = asset->GetID();
        const auto path = util::normalize_asset_path(asset->GetPath());
        assetsById_[id] = std::move(asset);
        if (!path.empty())
        {
            idByPath_[path] = id;
        }

        EditorAssetRecord record;
        record.assetID = id;
        record.logicalPath = path;
        record.sourcePath = path;
        record.type = asset->GetType();
        UpsertAssetRecord(std::move(record));
    }

    bool EditorAssetManager::RemoveAsset(const algorithm::UUID& id)
    {
        auto it = assetsById_.find(id);
        if (it == assetsById_.end())
        {
            return false;
        }
        const auto path = util::normalize_asset_path(it->second->GetPath());
        if (!path.empty())
        {
            idByPath_.erase(path);
        }
        recordsById_.erase(id);
        idByLogicalPath_.erase(path);
        idBySourcePath_.erase(path);
        idByPackagePath_.erase(path);
        assetsById_.erase(it);
        return true;
    }

    std::shared_ptr<IAssetBase> EditorAssetManager::GetAsset(const algorithm::UUID& id) const
    {
        auto it = assetsById_.find(id);
        if (it == assetsById_.end())
        {
            return nullptr;
        }
        return it->second;
    }

    std::shared_ptr<IAssetBase> EditorAssetManager::GetAssetByPath(const std::string& path) const
    {
        auto it = idByPath_.find(util::normalize_asset_path(path));
        if (it == idByPath_.end())
        {
            return nullptr;
        }
        return GetAsset(it->second);
    }

    std::vector<std::shared_ptr<IAssetBase>> EditorAssetManager::GetAllAssets() const
    {
        std::vector<std::shared_ptr<IAssetBase>> assets;
        assets.reserve(assetsById_.size());
        for (const auto& [id, asset] : assetsById_)
        {
            (void)id;
            assets.push_back(asset);
        }
        return assets;
    }

    bool EditorAssetManager::UpsertAssetRecord(EditorAssetRecord record)
    {
        if (record.assetID.IsZero())
        {
            return false;
        }

        record.logicalPath = util::normalize_asset_path(record.logicalPath);
        record.sourcePath = util::normalize_asset_path(record.sourcePath);
        record.packagePath = util::normalize_asset_path(record.packagePath);

        if (auto it = recordsById_.find(record.assetID); it != recordsById_.end())
        {
            idByLogicalPath_.erase(it->second.logicalPath);
            idBySourcePath_.erase(it->second.sourcePath);
            idByPackagePath_.erase(it->second.packagePath);
        }

        if (!record.logicalPath.empty())
        {
            idByLogicalPath_[record.logicalPath] = record.assetID;
        }
        if (!record.sourcePath.empty())
        {
            idBySourcePath_[record.sourcePath] = record.assetID;
        }
        if (!record.packagePath.empty())
        {
            idByPackagePath_[record.packagePath] = record.assetID;
        }
        recordsById_[record.assetID] = std::move(record);
        return true;
    }

    std::expected<EditorAssetRecord, std::string> EditorAssetManager::GetAssetRecord(const algorithm::UUID& id) const
    {
        auto it = recordsById_.find(id);
        if (it == recordsById_.end())
        {
            return std::unexpected("资产记录不存在");
        }
        return it->second;
    }

    std::expected<EditorAssetRecord, std::string> EditorAssetManager::GetAssetRecordByPath(const std::string& anyPath) const
    {
        const auto normalized = util::normalize_asset_path(anyPath);
        if (auto it = idByLogicalPath_.find(normalized); it != idByLogicalPath_.end())
        {
            return GetAssetRecord(it->second);
        }
        if (auto it = idBySourcePath_.find(normalized); it != idBySourcePath_.end())
        {
            return GetAssetRecord(it->second);
        }
        if (auto it = idByPackagePath_.find(normalized); it != idByPackagePath_.end())
        {
            return GetAssetRecord(it->second);
        }
        return std::unexpected("路径未注册为编辑器资产");
    }

    std::vector<EditorAssetRecord> EditorAssetManager::GetAllAssetRecords() const
    {
        std::vector<EditorAssetRecord> records;
        records.reserve(recordsById_.size());
        for (const auto& [id, record] : recordsById_)
        {
            (void)id;
            records.push_back(record);
        }
        return records;
    }

    bool EditorAssetManager::AddDependency(const algorithm::UUID& owner, const algorithm::UUID& dependency, bool hardReference)
    {
        auto ownerIt = recordsById_.find(owner);
        if (ownerIt == recordsById_.end() || recordsById_.find(dependency) == recordsById_.end())
        {
            return false;
        }
        auto& deps = ownerIt->second.dependencies;
        auto it = std::find_if(deps.begin(), deps.end(), [&](const EditorAssetDependency& dep) { return dep.assetID == dependency; });
        if (it != deps.end())
        {
            it->hardReference = hardReference;
            return true;
        }
        EditorAssetDependency dep;
        dep.assetID = dependency;
        dep.hardReference = hardReference;
        deps.push_back(std::move(dep));
        return true;
    }

    std::vector<algorithm::UUID> EditorAssetManager::BuildPreloadList(const std::vector<algorithm::UUID>& roots) const
    {
        std::vector<algorithm::UUID> result;
        std::unordered_set<algorithm::UUID, algorithm::UUID::Hash> visited;
        std::queue<algorithm::UUID> queue;

        for (const auto& root : roots)
        {
            if (root.IsZero() || !visited.insert(root).second)
            {
                continue;
            }
            queue.push(root);
        }

        while (!queue.empty())
        {
            auto id = queue.front();
            queue.pop();
            auto it = recordsById_.find(id);
            if (it == recordsById_.end())
            {
                continue;
            }
            result.push_back(id);
            for (const auto& dep : it->second.dependencies)
            {
                if (!dep.hardReference)
                {
                    continue;
                }
                if (visited.insert(dep.assetID).second)
                {
                    queue.push(dep.assetID);
                }
            }
        }
        return result;
    }

    std::expected<void, std::string> EditorAssetManager::ValidateForCook(const algorithm::UUID& id) const
    {
        auto recordResult = GetAssetRecord(id);
        if (!recordResult.has_value())
        {
            return std::unexpected(recordResult.error());
        }
        const auto& record = recordResult.value();
        if (record.type == EEditorAssetType::Unknown)
        {
            return std::unexpected("资产类型未知，无法Cook");
        }
        if (record.sourcePath.empty() && record.packagePath.empty())
        {
            return std::unexpected("缺少源路径与包路径");
        }
        for (const auto& dep : record.dependencies)
        {
            if (dep.hardReference && recordsById_.find(dep.assetID) == recordsById_.end())
            {
                return std::unexpected(fmt::format("硬依赖缺失: {}", dep.assetID.ToStringCompact()));
            }
        }
        return {};
    }

    std::unordered_map<uint32_t, std::vector<algorithm::UUID>> EditorAssetManager::BuildBundleIndex() const
    {
        std::unordered_map<uint32_t, std::vector<algorithm::UUID>> result;
        for (const auto& [id, record] : recordsById_)
        {
            if (record.buildState.bundleId == 0)
            {
                continue;
            }
            result[record.buildState.bundleId].push_back(id);
        }
        return result;
    }

    std::vector<std::string> EditorAssetManager::BuildCookManifest() const
    {
        std::vector<std::string> lines;
        lines.reserve(recordsById_.size());
        for (const auto& [id, record] : recordsById_)
        {
            lines.push_back(fmt::format("{}|{}|{}|{}|{}|{}",
                id.ToStringCompact(),
                static_cast<int>(record.type),
                record.logicalPath,
                record.sourcePath,
                record.packagePath,
                record.buildState.bundleId));
        }
        return lines;
    }

}
