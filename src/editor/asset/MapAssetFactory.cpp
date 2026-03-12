#include "editor/asset/MapAssetFactory.h"

#include <memory>

#include "gameplay/world/map_asset.h"
#include "util/path_util.h"

namespace shine::editor::asset
{
    namespace
    {
        [[nodiscard]] shine::SString NormalizeMapLogicalPath(shine::STextView path)
        {
            if (path.empty())
            {
                return {"/game/maps/untitled"};
            }

            shine::SString normalized = shine::util::normalize_asset_path(shine::SString::from_view(path));
            normalized.replace_inplace("\\", "/");

            if (normalized.empty())
            {
                return {"/game/maps/untitled"};
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

        [[nodiscard]] shine::SString MakeDefaultMapName(shine::STextView logicalPath)
        {
            const shine::SString normalized = NormalizeMapLogicalPath(logicalPath);
            const auto lastSlash = normalized.rfind('/');
            if (lastSlash == shine::SString::npos || lastSlash + 1 >= normalized.size())
            {
                return {"UntitledMap"};
            }

            shine::SString leaf = normalized.substr(lastSlash + 1);
            if (leaf.empty())
            {
                return {"UntitledMap"};
            }

            return leaf;
        }

        [[nodiscard]] shine::SString NormalizeSourcePathIfAny(shine::STextView sourcePath)
        {
            if (sourcePath.empty())
            {
                return {};
            }

            return shine::util::normalize_path(shine::SString::from_view(sourcePath));
        }
    }

    shine::AssetCreateResult MapAssetFactory::CreateAsset(const shine::AssetCreateContext& context)
    {
        shine::AssetCreateResult result;

        if (context.kind != shine::EAssetKind::World)
        {
            result.errorMessage = "MapAssetFactory only supports world assets";
            return result;
        }

        const shine::SString logicalPath = NormalizeMapLogicalPath(context.logicalPath);
        const shine::SString assetName = context.assetName.empty()
            ? MakeDefaultMapName(logicalPath.view())
            : shine::SString(context.assetName);

        auto mapAsset = std::make_shared<shine::gameplay::world::MapAsset>(assetName, logicalPath);
        mapAsset->SetSourcePath(NormalizeSourcePathIfAny(context.sourcePath));

        auto& worldSettings = mapAsset->GetWorldSettings();
        worldSettings.gravityZ = -980.0f;
        worldSettings.timeDilation = 1.0f;
        worldSettings.enableGlobalIllumination = true;

        auto& persistentLevel = mapAsset->GetPersistentLevel();
        persistentLevel.name = "PersistentLevel";
        persistentLevel.initiallyLoaded = true;
        persistentLevel.actorDefinitions.clear();

        mapAsset->MarkClean();

        result.asset = std::static_pointer_cast<shine::AssetBase>(mapAsset);
        return result;
    }

    shine::EAssetKind MapAssetFactory::GetSupportedKind() const noexcept
    {
        return shine::EAssetKind::World;
    }

    shine::STextView MapAssetFactory::GetDisplayName() const noexcept
    {
        return shine::STextView::from_literal("Map");
    }
}