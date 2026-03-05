#include "asset_base.h"
#include <algorithm>
#include "util/timer/timer_util.h"

namespace shine::editor::asset
{
    uint64_t IAssetBase::CurrentTimestamp()
    {
        return static_cast<uint64_t>(util::get_now_ms_platform<unsigned long long>());
    }

    void IAssetBase::Init(std::string name, std::string path, EEditorAssetType type)
    {
        assetID = algorithm::UUID::GenerateV7();
        assetName = std::move(name);
        assetPath = std::move(path);
        assetType = type;
        createTimestamp = CurrentTimestamp();
        modifiedTimestamp = createTimestamp;
        lifecycle = EAssetLifecycle::Imported;
        assetVersion = 1;
        dirty = false;
    }

    void IAssetBase::SetName(std::string name)
    {
        assetName = std::move(name);
        Touch();
    }

    void IAssetBase::SetPath(std::string path)
    {
        assetPath = std::move(path);
        Touch();
    }

    void IAssetBase::SetSourceHash(std::string md5)
    {
        assetMd5 = std::move(md5);
        Touch();
    }

    void IAssetBase::SetLifecycle(EAssetLifecycle state)
    {
        lifecycle = state;
        Touch();
    }

    void IAssetBase::SetImportOption(std::string key, std::string value)
    {
        importSettings.options[std::move(key)] = std::move(value);
        Touch();
    }

    std::optional<std::string> IAssetBase::GetImportOption(const std::string& key) const
    {
        auto it = importSettings.options.find(key);
        if (it == importSettings.options.end())
        {
            return std::nullopt;
        }
        return it->second;
    }

    void IAssetBase::SetCookProfile(AssetCookProfile profile)
    {
        cookProfile = std::move(profile);
        Touch();
    }

    void IAssetBase::AddDependency(const algorithm::UUID& dependencyID, std::string pathHint, bool hardReference)
    {
        auto it = std::find_if(dependencies.begin(), dependencies.end(),
            [&](const AssetDependencyRef& item) { return item.assetID == dependencyID; });
        if (it != dependencies.end())
        {
            it->pathHint = std::move(pathHint);
            it->hardReference = hardReference;
            Touch();
            return;
        }

        dependencies.push_back(AssetDependencyRef{
            .assetID = dependencyID,
            .pathHint = std::move(pathHint),
            .hardReference = hardReference
            });
        Touch();
    }

    bool IAssetBase::RemoveDependency(const algorithm::UUID& dependencyID)
    {
        const auto oldSize = dependencies.size();
        std::erase_if(dependencies, [&](const AssetDependencyRef& item) { return item.assetID == dependencyID; });
        const bool changed = oldSize != dependencies.size();
        if (changed)
        {
            Touch();
        }
        return changed;
    }

    void IAssetBase::ClearDependencies()
    {
        if (dependencies.empty())
        {
            return;
        }
        dependencies.clear();
        Touch();
    }

    void IAssetBase::Touch()
    {
        modifiedTimestamp = CurrentTimestamp();
        dirty = true;
        lifecycle = EAssetLifecycle::Dirty;
    }

    void IAssetBase::BumpVersion()
    {
        ++assetVersion;
        Touch();
    }

    const std::string& IAssetBase::GetName() const noexcept
    {
        return assetName;
    }

    const std::string& IAssetBase::GetPath() const noexcept
    {
        return assetPath;
    }

    const std::string& IAssetBase::GetSourceHash() const noexcept
    {
        return assetMd5;
    }

    const algorithm::UUID& IAssetBase::GetID() const noexcept
    {
        return assetID;
    }

    EEditorAssetType IAssetBase::GetType() const noexcept
    {
        return assetType;
    }

    EAssetLifecycle IAssetBase::GetLifecycle() const noexcept
    {
        return lifecycle;
    }

    uint32_t IAssetBase::GetVersion() const noexcept
    {
        return assetVersion;
    }

    uint64_t IAssetBase::GetCreateTimestamp() const noexcept
    {
        return createTimestamp;
    }

    uint64_t IAssetBase::GetModifiedTimestamp() const noexcept
    {
        return modifiedTimestamp;
    }

    const AssetImportSettings& IAssetBase::GetImportSettings() const noexcept
    {
        return importSettings;
    }

    const AssetCookProfile& IAssetBase::GetCookProfile() const noexcept
    {
        return cookProfile;
    }

    const std::vector<AssetDependencyRef>& IAssetBase::GetDependencies() const noexcept
    {
        return dependencies;
    }

    bool IAssetBase::IsDirty() const noexcept
    {
        return dirty;

    }
}
