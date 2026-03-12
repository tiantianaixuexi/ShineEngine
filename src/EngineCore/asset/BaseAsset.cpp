#include "BaseAsset.h"

#include <algorithm>
#include <utility>

#include "util/path_util.h"
#include "util/timer/TimerUtil.h"

namespace
{
    [[nodiscard]] shine::SString NormalizeLogicalAssetPath(shine::STextView path)
    {
        if (path.empty())
        {
            return {"/game/unnamed"};
        }

        shine::SString normalized = shine::util::normalize_asset_path(shine::SString::from_view(path));
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

    [[nodiscard]] shine::SString NormalizeSourceAssetPath(shine::STextView path)
    {
        if (path.empty())
        {
            return {};
        }

        return shine::util::normalize_path(shine::SString::from_view(path));
    }
}

namespace shine::editor::asset
{
    std::uint64_t AssetBase::CurrentTimestamp() noexcept
    {
        return static_cast<std::uint64_t>(util::get_now_ms_platform<unsigned long long>());
    }

    void AssetBase::Init(
        shine::AssetID id,
        shine::SString name,
        const shine::SString& logicalPath,
        shine::EAssetKind kind)
    {
        assetID_ = id;
        assetName_ = std::move(name);
        logicalPath_ = NormalizeLogicalAssetPath(logicalPath.view());
        sourcePath_.clear();
        sourceHash_.clear();
        assetKind_ = kind;
        lifecycle_ = shine::EAssetLifecycle::Imported;
        importSettings_ = {};
        cookProfile_ = {};
        dependencies_.clear();
        assetVersion_ = 1;
        createTimestamp_ = CurrentTimestamp();
        modifiedTimestamp_ = createTimestamp_;
        dirty_ = false;
    }

    void AssetBase::SetID(shine::AssetID id) noexcept
    {
        if (assetID_ == id)
        {
            return;
        }

        assetID_ = id;
        MarkDirty();
    }

    void AssetBase::SetName(shine::SString name)
    {
        if (assetName_ == name)
        {
            return;
        }

        assetName_ = std::move(name);
        MarkDirty();
    }

    void AssetBase::SetLogicalPath(const shine::SString& logicalPath)
    {
        shine::SString normalized = NormalizeLogicalAssetPath(logicalPath.view());
        if (logicalPath_ == normalized)
        {
            return;
        }

        logicalPath_ = std::move(normalized);
        MarkDirty();
    }

    void AssetBase::SetSourcePath(const shine::SString& sourcePath)
    {
        shine::SString normalized = NormalizeSourceAssetPath(sourcePath.view());
        if (sourcePath_ == normalized)
        {
            return;
        }

        sourcePath_ = std::move(normalized);
        Touch();
    }

    void AssetBase::SetSourceHash(shine::SString hash)
    {
        if (sourceHash_ == hash)
        {
            return;
        }

        sourceHash_ = std::move(hash);
        Touch();
    }

    void AssetBase::SetLifecycle(shine::EAssetLifecycle lifecycle) noexcept
    {
        if (lifecycle_ == lifecycle)
        {
            return;
        }

        lifecycle_ = lifecycle;
        Touch();
    }

    void AssetBase::SetImportSettings(AssetImportSettings settings)
    {
        importSettings_ = std::move(settings);
        MarkDirty();
    }

    void AssetBase::SetImportOption(shine::StringId key, shine::StringId value)
    {
        auto it = std::find_if(
            importSettings_.options.begin(),
            importSettings_.options.end(),
            [key](const auto& pair)
            {
                return pair.first == key;
            });

        if (it != importSettings_.options.end())
        {
            if (it->second == value)
            {
                return;
            }

            it->second = value;
        }
        else
        {
            importSettings_.options.emplace_back(key, value);
        }

        MarkDirty();
    }

    std::optional<shine::StringId> AssetBase::GetImportOption(shine::StringId key) const
    {
        const auto it = std::find_if(
            importSettings_.options.begin(),
            importSettings_.options.end(),
            [key](const auto& pair)
            {
                return pair.first == key;
            });

        if (it == importSettings_.options.end())
        {
            return std::nullopt;
        }

        return it->second;
    }

    void AssetBase::RemoveImportOption(shine::StringId key)
    {
        const auto oldSize = importSettings_.options.size();
        std::erase_if(
            importSettings_.options,
            [key](const auto& pair)
            {
                return pair.first == key;
            });

        if (oldSize != importSettings_.options.size())
        {
            MarkDirty();
        }
    }

    void AssetBase::ClearImportOptions()
    {
        if (importSettings_.options.empty())
        {
            return;
        }

        importSettings_.options.clear();
        MarkDirty();
    }

    void AssetBase::SetCookProfile(AssetCookProfile profile)
    {
        cookProfile_ = std::move(profile);
        Touch();
    }

    void AssetBase::AddDependency(
        shine::AssetID dependencyID,
        shine::StringId pathHintId,
        bool hardReference)
    {
        if (dependencyID == shine::InvalidAssetID || dependencyID == assetID_)
        {
            return;
        }

        auto it = std::find_if(
            dependencies_.begin(),
            dependencies_.end(),
            [dependencyID](const AssetDependencyRef& item)
            {
                return item.assetID == dependencyID;
            });

        if (it != dependencies_.end())
        {
            const bool changed =
                it->pathHintId != pathHintId ||
                it->hardReference != hardReference;

            if (!changed)
            {
                return;
            }

            it->pathHintId = pathHintId;
            it->hardReference = hardReference;
            MarkDirty();
            return;
        }

        dependencies_.push_back(AssetDependencyRef{
            .assetID = dependencyID,
            .pathHintId = pathHintId,
            .hardReference = hardReference
        });
        MarkDirty();
    }

    bool AssetBase::RemoveDependency(shine::AssetID dependencyID)
    {
        const auto oldSize = dependencies_.size();
        std::erase_if(
            dependencies_,
            [dependencyID](const AssetDependencyRef& item)
            {
                return item.assetID == dependencyID;
            });

        const bool changed = oldSize != dependencies_.size();
        if (changed)
        {
            MarkDirty();
        }

        return changed;
    }

    void AssetBase::ClearDependencies()
    {
        if (dependencies_.empty())
        {
            return;
        }

        dependencies_.clear();
        MarkDirty();
    }

    void AssetBase::MarkDirty() noexcept
    {
        modifiedTimestamp_ = CurrentTimestamp();
        dirty_ = true;

        if (lifecycle_ != shine::EAssetLifecycle::Cooking)
        {
            lifecycle_ = shine::EAssetLifecycle::Dirty;
        }
    }

    void AssetBase::MarkClean() noexcept
    {
        dirty_ = false;
        if (lifecycle_ == shine::EAssetLifecycle::Dirty)
        {
            lifecycle_ = shine::EAssetLifecycle::Imported;
        }
        modifiedTimestamp_ = CurrentTimestamp();
    }

    void AssetBase::Touch() noexcept
    {
        modifiedTimestamp_ = CurrentTimestamp();
    }

    void AssetBase::BumpVersion()
    {
        ++assetVersion_;
        MarkDirty();
    }

    shine::STextView AssetBase::GetClassName() const noexcept
    {
        return shine::STextView::from_literal("AssetBase");
    }

    shine::AssetID AssetBase::GetID() const noexcept
    {
        return assetID_;
    }

    shine::STextView AssetBase::GetName() const noexcept
    {
        return assetName_.view();
    }

    shine::STextView AssetBase::GetLogicalPath() const noexcept
    {
        return logicalPath_.view();
    }

    shine::STextView AssetBase::GetSourcePath() const noexcept
    {
        return sourcePath_.view();
    }

    shine::STextView AssetBase::GetSourceHash() const noexcept
    {
        return sourceHash_.view();
    }

    shine::EAssetKind AssetBase::GetKind() const noexcept
    {
        return assetKind_;
    }

    shine::EAssetLifecycle AssetBase::GetLifecycle() const noexcept
    {
        return lifecycle_;
    }

    std::uint32_t AssetBase::GetVersion() const noexcept
    {
        return assetVersion_;
    }

    std::uint64_t AssetBase::GetCreateTimestamp() const noexcept
    {
        return createTimestamp_;
    }

    std::uint64_t AssetBase::GetModifiedTimestamp() const noexcept
    {
        return modifiedTimestamp_;
    }

    const AssetImportSettings& AssetBase::GetImportSettings() const noexcept
    {
        return importSettings_;
    }

    const AssetCookProfile& AssetBase::GetCookProfile() const noexcept
    {
        return cookProfile_;
    }

    const std::vector<AssetDependencyRef>& AssetBase::GetDependencies() const noexcept
    {
        return dependencies_;
    }

    bool AssetBase::IsDirty() const noexcept
    {
        return dirty_;
    }

    bool AssetBase::HasValidID() const noexcept
    {
        return assetID_ != shine::InvalidAssetID;
    }
}
