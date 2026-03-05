#include "map_asset.h"

namespace shine::gameplay::world
{
    MapAsset::MapAsset(std::string mapName, std::string mapPath)
        : name_(std::move(mapName))
    {
        Init(name_, std::move(mapPath), shine::editor::asset::EEditorAssetType::Scene);
        persistentLevel_.name = "PersistentLevel";
        persistentLevel_.initiallyLoaded = true;
        persistentLevel_.state = LevelAsset::StreamingState::Loaded;
    }

    LevelAsset* MapAsset::findStreamingLevel(std::string_view levelName) noexcept
    {
        for (auto& level : streamingLevels_)
        {
            if (level.name == levelName)
            {
                return &level;
            }
        }
        return nullptr;
    }

    const LevelAsset* MapAsset::findStreamingLevel(std::string_view levelName) const noexcept
    {
        for (const auto& level : streamingLevels_)
        {
            if (level.name == levelName)
            {
                return &level;
            }
        }
        return nullptr;
    }

    LevelAsset& MapAsset::addStreamingLevel(const std::string& levelName)
    {
        if (auto* level = findStreamingLevel(levelName))
        {
            return *level;
        }
        streamingLevels_.push_back(LevelAsset{});
        auto& created = streamingLevels_.back();
        created.name = levelName;
        return created;
    }
}
