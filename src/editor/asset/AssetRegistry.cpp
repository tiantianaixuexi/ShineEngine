#include "editor/asset/AssetRegistry.h"

#include <algorithm>
#include <queue>
#include <unordered_set>
#include <utility>

#include "util/path_util.h"

namespace shine::editor::asset
{
    namespace
    {
        [[nodiscard]] SString NormalizeLogicalPath(STextView path)
        {
            if (path.empty())
            {
                return {"/game/unnamed"};
            }

            SString normalized = util::normalize_asset_path(SString::from_view(path));
            normalized.replace_inplace("\\", "/");

            if (normalized.empty())
            {
                return {"/game/unnamed"};
            }

            if (!normalized.starts_with("/"))
            {
                normalized.insert(0, "/");
            }

            while (normalized.contains("//"))
            {
                normalized.replace_inplace("//", "/");
            }

            if (normalized.size() > 1 && normalized.ends_with("/"))
            {
                normalized.erase(normalized.size() - 1, 1);
            }

            return normalized;
        }

        [[nodiscard]] SString NormalizeSourcePath(STextView path)
        {
            if (path.empty())
            {
                return {};
            }

            return util::normalize_path(SString::from_view(path));
        }

        [[nodiscard]] SString MakeErrorText(STextView text)
        {
            return SString::from_view(text);
        }
    }

    bool AssetRegistry::Init(EngineContext& ctx)
    {
        (void)ctx;
        Clear();
        return true;
    }

    void AssetRegistry::Shutdown(EngineContext& ctx)
    {
        (void)ctx;
        Clear();
    }

    void AssetRegistry::Clear()
    {
        assetsById_.clear();
        records_.clear();
        idToIndexMap_.clear();
        handleByLogicalPath_.clear();
        handlesByKind_.clear();
        dependencyPool_.clear();
        stringPool_.clear();
        stringToId_.clear();
        factories_.clear();
    }

    void AssetRegistry::RegisterAsset(const std::shared_ptr<AssetBase>& asset)
    {
        if (!asset || !asset->HasValidID())
        {
            return;
        }

        assetsById_[asset->GetID()] = asset;

        AssetRegistryRecord record;
        record.assetID = asset->GetID();
        record.nameId = InternString(asset->GetName());
        record.logicalPathId = NormalizeAndInternLogicalPath(asset->GetLogicalPath());
        record.sourcePathId = InternString(NormalizeSourcePath(asset->GetSourcePath()).view());
        record.kind = asset->GetKind();
        record.buildState.version = asset->GetVersion();
        record.buildState.bundleId = asset->GetCookProfile().bundleId;
        record.buildState.cooked = asset->GetLifecycle() == shine::EAssetLifecycle::Cooked;

        if (const AssetRegistryRecord* existing = FindRecordPtr(asset->GetID()))
        {
            record.dependencyOffset = existing->dependencyOffset;
            record.dependencyCount = existing->dependencyCount;
        }
        else
        {
            const auto& dependencies = asset->GetDependencies();
            if (!dependencies.empty())
            {
                record.dependencyOffset = static_cast<std::uint32_t>(dependencyPool_.size());
                record.dependencyCount = static_cast<std::uint32_t>(dependencies.size());

                for (const auto& dependency : dependencies)
                {
                    dependencyPool_.push_back(AssetRegistryDependency{
                        .assetID = dependency.assetID,
                        .hardReference = dependency.hardReference
                    });
                }
            }
        }

        UpsertRecord(record);
    }

    bool AssetRegistry::RemoveAsset(shine::AssetID assetID)
    {
        const auto handle = FindRecordHandle(assetID);
        if (handle == shine::InvalidHandle)
        {
            return false;
        }

        assetsById_.erase(assetID);
        records_.erase(records_.begin() + static_cast<std::ptrdiff_t>(handle));
        RebuildRecordLookupTables();

        return true;
    }

    bool AssetRegistry::ContainsAsset(shine::AssetID assetID) const noexcept
    {
        return FindRecordHandle(assetID) != shine::InvalidHandle;
    }

    bool AssetRegistry::ContainsPath(shine::STextView logicalPath) const noexcept
    {
        const auto pathId = FindLogicalPathId(logicalPath);
        return pathId != shine::InvalidStringId
            && handleByLogicalPath_.find(pathId) != handleByLogicalPath_.end();
    }

    std::shared_ptr<AssetBase> AssetRegistry::GetAsset(shine::AssetID assetID) const
    {
        if (const auto it = assetsById_.find(assetID); it != assetsById_.end())
        {
            return it->second;
        }

        return nullptr;
    }

    std::shared_ptr<AssetBase> AssetRegistry::GetAssetByLogicalPath(shine::STextView logicalPath) const noexcept
    {
        const auto pathId = FindLogicalPathId(logicalPath);
        if (pathId == shine::InvalidStringId)
        {
            return nullptr;
        }

        const auto it = handleByLogicalPath_.find(pathId);
        if (it == handleByLogicalPath_.end())
        {
            return nullptr;
        }

        return GetAsset(records_[it->second].assetID);
    }

    std::expected<AssetRegistryRecord, shine::SString> AssetRegistry::GetRecord(shine::AssetID assetID) const
    {
        if (const auto* record = FindRecordPtr(assetID))
        {
            return *record;
        }

        return std::unexpected(MakeErrorText("Asset record not found"));
    }

    std::expected<AssetRegistryRecord, shine::SString> AssetRegistry::GetRecordByLogicalPath(shine::STextView logicalPath) const noexcept
    {
        const auto pathId = FindLogicalPathId(logicalPath);
        if (pathId == shine::InvalidStringId)
        {
            return std::unexpected(MakeErrorText("Asset logical path not registered"));
        }

        const auto it = handleByLogicalPath_.find(pathId);
        if (it == handleByLogicalPath_.end())
        {
            return std::unexpected(MakeErrorText("Asset logical path not registered"));
        }

        return records_[it->second];
    }

    std::vector<std::shared_ptr<AssetBase>> AssetRegistry::GetAssetsByKind(shine::EAssetKind kind) const
    {
        std::vector<std::shared_ptr<AssetBase>> result;

        const auto it = handlesByKind_.find(kind);
        if (it == handlesByKind_.end())
        {
            return result;
        }

        result.reserve(it->second.size());
        for (const auto handle : it->second)
        {
            if (handle >= records_.size())
            {
                continue;
            }

            if (auto asset = GetAsset(records_[handle].assetID))
            {
                result.push_back(std::move(asset));
            }
        }

        return result;
    }

    std::vector<AssetRegistryRecord> AssetRegistry::GetRecordsByKind(shine::EAssetKind kind) const noexcept
    {
        std::vector<AssetRegistryRecord> result;

        const auto it = handlesByKind_.find(kind);
        if (it == handlesByKind_.end())
        {
            return result;
        }

        result.reserve(it->second.size());
        for (const auto handle : it->second)
        {
            if (handle < records_.size())
            {
                result.push_back(records_[handle]);
            }
        }

        return result;
    }

    const std::vector<AssetRegistryRecord>& AssetRegistry::GetAllRecords() const noexcept
    {
        return records_;
    }

    std::vector<std::shared_ptr<AssetBase>> AssetRegistry::GetAllAssets() const
    {
        std::vector<std::shared_ptr<AssetBase>> result;
        result.reserve(records_.size());

        for (const auto& record : records_)
        {
            if (auto asset = GetAsset(record.assetID))
            {
                result.push_back(std::move(asset));
            }
        }

        return result;
    }

    bool AssetRegistry::AddDependency(
        shine::AssetID ownerAssetID,
        shine::AssetID dependencyAssetID,
        bool hardReference)
    {
        AssetRegistryRecord* record = FindMutableRecord(ownerAssetID);
        if (record == nullptr || dependencyAssetID == shine::InvalidAssetID || dependencyAssetID == ownerAssetID)
        {
            return false;
        }

        std::vector<AssetRegistryDependency> dependencies = GetDependencies(ownerAssetID);
        const auto it = std::ranges::find(
            dependencies,
            dependencyAssetID,
            &AssetRegistryDependency::assetID);

        if (it != dependencies.end())
        {
            if (it->hardReference == hardReference)
            {
                return true;
            }

            it->hardReference = hardReference;
        }
        else
        {
            dependencies.push_back(AssetRegistryDependency{
                .assetID = dependencyAssetID,
                .hardReference = hardReference
            });
        }

        RebuildDependencyPoolForAsset(ownerAssetID, dependencies);

        if (auto asset = GetAsset(ownerAssetID))
        {
            asset->AddDependency(dependencyAssetID, shine::InvalidStringId, hardReference);
        }

        return true;
    }

    bool AssetRegistry::RemoveDependency(
        shine::AssetID ownerAssetID,
        shine::AssetID dependencyAssetID)
    {
        AssetRegistryRecord* record = FindMutableRecord(ownerAssetID);
        if (record == nullptr)
        {
            return false;
        }

        std::vector<AssetRegistryDependency> dependencies = GetDependencies(ownerAssetID);
        const auto oldSize = dependencies.size();

        std::erase_if(
            dependencies,
            [dependencyAssetID](const AssetRegistryDependency& dependency)
            {
                return dependency.assetID == dependencyAssetID;
            });

        if (oldSize == dependencies.size())
        {
            return false;
        }

        RebuildDependencyPoolForAsset(ownerAssetID, dependencies);

        if (auto asset = GetAsset(ownerAssetID))
        {
            [[maybe_unused]] const bool removed = asset->RemoveDependency(dependencyAssetID);
            (void)removed;
        }

        return true;
    }

    void AssetRegistry::ClearDependencies(shine::AssetID ownerAssetID)
    {
        AssetRegistryRecord* record = FindMutableRecord(ownerAssetID);
        if (record == nullptr)
        {
            return;
        }

        record->dependencyOffset = 0;
        record->dependencyCount = 0;

        if (auto asset = GetAsset(ownerAssetID))
        {
            [[maybe_unused]] const bool hadValidId = asset->HasValidID();
            asset->ClearDependencies();
            (void)hadValidId;
        }

        RebuildRecordLookupTables();
    }

    std::vector<AssetRegistryDependency> AssetRegistry::GetDependencies(shine::AssetID assetID) const
    {
        std::vector<AssetRegistryDependency> result;

        const auto* record = FindRecordPtr(assetID);
        if (record == nullptr || record->dependencyCount == 0)
        {
            return result;
        }

        const auto start = static_cast<std::size_t>(record->dependencyOffset);
        const auto count = static_cast<std::size_t>(record->dependencyCount);

        if (start >= dependencyPool_.size() || start + count > dependencyPool_.size())
        {
            return result;
        }

        result.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
            result.push_back(dependencyPool_[start + i]);
        }

        return result;
    }

    std::vector<shine::AssetID> AssetRegistry::GetReferencers(shine::AssetID assetID) const
    {
        std::vector<shine::AssetID> result;

        for (const auto& record : records_)
        {
            if (record.assetID == assetID || record.dependencyCount == 0)
            {
                continue;
            }

            const auto start = static_cast<std::size_t>(record.dependencyOffset);
            const auto count = static_cast<std::size_t>(record.dependencyCount);
            if (start >= dependencyPool_.size() || start + count > dependencyPool_.size())
            {
                continue;
            }

            bool found = false;
            for (std::size_t i = 0; i < count; ++i)
            {
                if (dependencyPool_[start + i].assetID == assetID)
                {
                    found = true;
                    break;
                }
            }

            if (found)
            {
                result.push_back(record.assetID);
            }
        }

        return result;
    }

    std::vector<shine::AssetID> AssetRegistry::BuildPreloadList(const std::vector<shine::AssetID>& roots) const
    {
        std::vector<shine::AssetID> result;
        std::queue<shine::AssetID> pending;
        std::unordered_set<shine::AssetID> visited;

        for (const auto root : roots)
        {
            if (root != shine::InvalidAssetID && visited.insert(root).second)
            {
                pending.push(root);
            }
        }

        while (!pending.empty())
        {
            const auto current = pending.front();
            pending.pop();

            result.push_back(current);

            const auto dependencies = GetDependencies(current);
            for (const auto& dependency : dependencies)
            {
                if (!dependency.hardReference)
                {
                    continue;
                }

                if (dependency.assetID != shine::InvalidAssetID && visited.insert(dependency.assetID).second)
                {
                    pending.push(dependency.assetID);
                }
            }
        }

        return result;
    }

    std::expected<void, shine::SString> AssetRegistry::ValidateForCook(shine::AssetID assetID) const
    {
        const auto* record = FindRecordPtr(assetID);
        if (record == nullptr)
        {
            return std::unexpected(MakeErrorText("Asset record not found"));
        }

        const auto dependencies = GetDependencies(assetID);
        for (const auto& dependency : dependencies)
        {
            if (!dependency.hardReference)
            {
                continue;
            }

            if (!ContainsAsset(dependency.assetID))
            {
                return std::unexpected(MakeErrorText("Missing hard dependency"));
            }
        }

        return {};
    }

    void AssetRegistry::RegisterFactory(std::unique_ptr<shine::IAssetFactory> factory)
    {
        if (!factory)
        {
            return;
        }

        factories_[factory->GetSupportedKind()] = std::move(factory);
    }

    bool AssetRegistry::HasFactory(shine::EAssetKind kind) const noexcept
    {
        return factories_.find(kind) != factories_.end();
    }

    std::shared_ptr<AssetBase> AssetRegistry::CreateAsset(const shine::AssetCreateContext& context)
    {
        const auto it = factories_.find(context.kind);
        if (it == factories_.end() || !it->second)
        {
            return nullptr;
        }

        auto result = it->second->CreateAsset(context);
        if (!result.Succeeded())
        {
            return nullptr;
        }

        auto asset = std::static_pointer_cast<AssetBase>(result.asset);
        RegisterAsset(asset);
        return asset;
    }

    shine::StringId AssetRegistry::InternString(shine::STextView text)
    {
        return InternStringInternal(text);
    }

    shine::StringId AssetRegistry::FindInternedString(shine::STextView text) const noexcept
    {
        return FindInternedStringInternal(text);
    }

    shine::STextView AssetRegistry::ResolveString(shine::StringId id) const noexcept
    {
        if (id == shine::InvalidStringId || id >= stringPool_.size())
        {
            return {};
        }

        return stringPool_[id].view();
    }

    std::size_t AssetRegistry::GetAssetCount() const noexcept
    {
        return records_.size();
    }

    std::size_t AssetRegistry::GetDependencyCount() const noexcept
    {
        return dependencyPool_.size();
    }

    shine::StringId AssetRegistry::NormalizeAndInternLogicalPath(shine::STextView logicalPath)
    {
        return InternStringInternal(NormalizeLogicalPath(logicalPath).view());
    }

    shine::StringId AssetRegistry::FindLogicalPathId(shine::STextView logicalPath) const noexcept
    {
        if (logicalPath.empty())
        {
            return shine::InvalidStringId;
        }

        const auto normalized = NormalizeLogicalPath(logicalPath);
        return FindInternedStringInternal(normalized.view());
    }

    shine::StringId AssetRegistry::InternStringInternal(shine::STextView text)
    {
        if (text.empty())
        {
            return shine::InvalidStringId;
        }

        const std::string_view key(text.data(), text.size());
        if (const auto it = stringToId_.find(key); it != stringToId_.end())
        {
            return it->second;
        }

        const auto id = static_cast<shine::StringId>(stringPool_.size());
        stringPool_.push_back(SString::from_view(text));
        const auto storedView = std::string_view(stringPool_.back().data(), stringPool_.back().size());
        stringToId_[storedView] = id;
        return id;
    }

    shine::StringId AssetRegistry::FindInternedStringInternal(shine::STextView text) const noexcept
    {
        if (text.empty())
        {
            return shine::InvalidStringId;
        }

        const std::string_view key(text.data(), text.size());
        if (const auto it = stringToId_.find(key); it != stringToId_.end())
        {
            return it->second;
        }

        return shine::InvalidStringId;
    }

    shine::AssetHandle AssetRegistry::FindRecordHandle(shine::AssetID assetID) const noexcept
    {
        const auto it = std::ranges::lower_bound(
            idToIndexMap_,
            assetID,
            {},
            &IdToIndex::id);

        if (it == idToIndexMap_.end() || it->id != assetID)
        {
            return shine::InvalidHandle;
        }

        return it->handle;
    }

    AssetRegistryRecord* AssetRegistry::FindMutableRecord(shine::AssetID assetID) noexcept
    {
        const auto handle = FindRecordHandle(assetID);
        if (handle == shine::InvalidHandle || handle >= records_.size())
        {
            return nullptr;
        }

        return &records_[handle];
    }

    const AssetRegistryRecord* AssetRegistry::FindRecordPtr(shine::AssetID assetID) const noexcept
    {
        const auto handle = FindRecordHandle(assetID);
        if (handle == shine::InvalidHandle || handle >= records_.size())
        {
            return nullptr;
        }

        return &records_[handle];
    }

    bool AssetRegistry::UpsertRecord(const AssetRegistryRecord& record)
    {
        if (!record.IsValid())
        {
            return false;
        }

        const auto it = std::ranges::lower_bound(
            idToIndexMap_,
            record.assetID,
            {},
            &IdToIndex::id);

        if (it != idToIndexMap_.end() && it->id == record.assetID)
        {
            records_[it->handle] = record;
            RebuildRecordLookupTables();
            return true;
        }

        records_.push_back(record);
        RebuildRecordLookupTables();
        return true;
    }

    void AssetRegistry::RebuildRecordLookupTables()
    {
        idToIndexMap_.clear();
        handleByLogicalPath_.clear();
        handlesByKind_.clear();

        idToIndexMap_.reserve(records_.size());

        for (shine::AssetHandle handle = 0; handle < records_.size(); ++handle)
        {
            const auto& record = records_[handle];
            idToIndexMap_.push_back(IdToIndex{
                .id = record.assetID,
                .handle = handle
            });

            if (record.logicalPathId != shine::InvalidStringId)
            {
                handleByLogicalPath_[record.logicalPathId] = handle;
            }

            handlesByKind_[record.kind].push_back(handle);
        }

        std::ranges::sort(
            idToIndexMap_,
            {},
            &IdToIndex::id);

        std::vector<AssetRegistryDependency> rebuiltDependencyPool;
        rebuiltDependencyPool.reserve(dependencyPool_.size());

        for (auto& record : records_)
        {
            std::vector<AssetRegistryDependency> dependencies;

            if (record.dependencyCount > 0)
            {
                const auto oldOffset = static_cast<std::size_t>(record.dependencyOffset);
                const auto oldCount = static_cast<std::size_t>(record.dependencyCount);

                if (oldOffset < dependencyPool_.size() && oldOffset + oldCount <= dependencyPool_.size())
                {
                    dependencies.reserve(oldCount);
                    for (std::size_t i = 0; i < oldCount; ++i)
                    {
                        dependencies.push_back(dependencyPool_[oldOffset + i]);
                    }
                }
            }

            if (dependencies.empty())
            {
                record.dependencyOffset = 0;
                record.dependencyCount = 0;
                continue;
            }

            record.dependencyOffset = static_cast<std::uint32_t>(rebuiltDependencyPool.size());
            record.dependencyCount = static_cast<std::uint32_t>(dependencies.size());
            rebuiltDependencyPool.insert(
                rebuiltDependencyPool.end(),
                dependencies.begin(),
                dependencies.end());
        }

        dependencyPool_ = std::move(rebuiltDependencyPool);
    }

    void AssetRegistry::RebuildDependencyPoolForAsset(
        shine::AssetID ownerAssetID,
        const std::vector<AssetRegistryDependency>& dependencies)
    {
        const auto ownerHandle = FindRecordHandle(ownerAssetID);
        if (ownerHandle == shine::InvalidHandle || ownerHandle >= records_.size())
        {
            return;
        }

        std::vector<AssetRegistryDependency> rebuiltPool;
        rebuiltPool.reserve(
            dependencyPool_.size()
            - static_cast<std::size_t>(records_[ownerHandle].dependencyCount)
            + dependencies.size());

        for (shine::AssetHandle handle = 0; handle < records_.size(); ++handle)
        {
            auto& record = records_[handle];

            std::vector<AssetRegistryDependency> currentDependencies;
            if (handle == ownerHandle)
            {
                currentDependencies = dependencies;
            }
            else if (record.dependencyCount > 0)
            {
                const auto start = static_cast<std::size_t>(record.dependencyOffset);
                const auto count = static_cast<std::size_t>(record.dependencyCount);

                if (start < dependencyPool_.size() && start + count <= dependencyPool_.size())
                {
                    currentDependencies.reserve(count);
                    for (std::size_t i = 0; i < count; ++i)
                    {
                        currentDependencies.push_back(dependencyPool_[start + i]);
                    }
                }
            }

            if (currentDependencies.empty())
            {
                record.dependencyOffset = 0;
                record.dependencyCount = 0;
                continue;
            }

            record.dependencyOffset = static_cast<std::uint32_t>(rebuiltPool.size());
            record.dependencyCount = static_cast<std::uint32_t>(currentDependencies.size());

            rebuiltPool.insert(
                rebuiltPool.end(),
                currentDependencies.begin(),
                currentDependencies.end());
        }

        dependencyPool_ = std::move(rebuiltPool);
    }
}