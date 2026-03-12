#include "map_asset.h"

#include <utility>

namespace shine::gameplay::world
{
    namespace
    {
        [[nodiscard]] bool AreLevelNamesEqual(const shine::SString& lhs, shine::STextView rhs) noexcept
        {
            return lhs.view() == rhs;
        }
    }

    LevelDefinition MapAsset::CreatePersistentLevelDefinition()
    {
        LevelDefinition level;
        level.name = "PersistentLevel";
        level.initiallyLoaded = true;
        return level;
    }

    bool MapAsset::RemoveActorByName(std::vector<ActorSpawnDefinition>& definitions, shine::STextView actorName)
    {
        const auto oldSize = definitions.size();
        std::erase_if(definitions,
            [actorName](const ActorSpawnDefinition& definition)
            {
                return definition.name.view() == actorName;
            });

        return oldSize != definitions.size();
    }

    MapAsset::MapAsset()
        : MapAsset("UntitledMap", "/game/maps/untitled")
    {
    }

    MapAsset::MapAsset(shine::SString mapName, const shine::SString& logicalPath)
        : persistentLevel_(CreatePersistentLevelDefinition())
    {
        Init(
            shine::InvalidAssetID,
            std::move(mapName),
            logicalPath,
            shine::EAssetKind::World);
    }

    WorldSettings& MapAsset::GetWorldSettings() noexcept
    {
        return worldSettings_;
    }

    const WorldSettings& MapAsset::GetWorldSettings() const noexcept
    {
        return worldSettings_;
    }

    void MapAsset::SetWorldSettings(const WorldSettings& settings)
    {
        worldSettings_ = settings;
        MarkDirty();
    }

    void MapAsset::SetGravityZ(float gravityZ)
    {
        if (worldSettings_.gravityZ == gravityZ)
        {
            return;
        }

        worldSettings_.gravityZ = gravityZ;
        MarkDirty();
    }

    void MapAsset::SetTimeDilation(float timeDilation)
    {
        if (worldSettings_.timeDilation == timeDilation)
        {
            return;
        }

        worldSettings_.timeDilation = timeDilation;
        MarkDirty();
    }

    void MapAsset::SetGlobalIlluminationEnabled(bool enabled)
    {
        if (worldSettings_.enableGlobalIllumination == enabled)
        {
            return;
        }

        worldSettings_.enableGlobalIllumination = enabled;
        MarkDirty();
    }

    LevelDefinition& MapAsset::GetPersistentLevel() noexcept
    {
        return persistentLevel_;
    }

    const LevelDefinition& MapAsset::GetPersistentLevel() const noexcept
    {
        return persistentLevel_;
    }

    std::vector<LevelDefinition>& MapAsset::GetStreamingLevels() noexcept
    {
        return streamingLevels_;
    }

    const std::vector<LevelDefinition>& MapAsset::GetStreamingLevels() const noexcept
    {
        return streamingLevels_;
    }

    LevelDefinition* MapAsset::FindStreamingLevel(shine::STextView levelName) noexcept
    {
        for (auto& level : streamingLevels_)
        {
            if (AreLevelNamesEqual(level.name, levelName))
            {
                return &level;
            }
        }

        return nullptr;
    }

    const LevelDefinition* MapAsset::FindStreamingLevel(shine::STextView levelName) const noexcept
    {
        for (const auto& level : streamingLevels_)
        {
            if (AreLevelNamesEqual(level.name, levelName))
            {
                return &level;
            }
        }

        return nullptr;
    }

    LevelDefinition& MapAsset::AddStreamingLevel(shine::SString levelName, bool initiallyLoaded)
    {
        if (auto* existing = FindStreamingLevel(levelName.view()))
        {
            return *existing;
        }

        LevelDefinition definition;
        definition.name = std::move(levelName);
        definition.initiallyLoaded = initiallyLoaded;

        streamingLevels_.push_back(std::move(definition));
        MarkDirty();
        return streamingLevels_.back();
    }

    bool MapAsset::RemoveStreamingLevel(shine::STextView levelName)
    {
        const auto oldSize = streamingLevels_.size();
        std::erase_if(streamingLevels_,
            [levelName](const LevelDefinition& level)
            {
                return level.name.view() == levelName;
            });

        const bool removed = oldSize != streamingLevels_.size();
        if (removed)
        {
            MarkDirty();
        }

        return removed;
    }

    void MapAsset::ClearStreamingLevels()
    {
        if (streamingLevels_.empty())
        {
            return;
        }

        streamingLevels_.clear();
        MarkDirty();
    }

    ActorSpawnDefinition& MapAsset::AddActorToPersistentLevel(const ActorSpawnDefinition& definition)
    {
        persistentLevel_.actorDefinitions.push_back(definition);
        MarkDirty();
        return persistentLevel_.actorDefinitions.back();
    }

    ActorSpawnDefinition& MapAsset::AddActorToStreamingLevel(shine::STextView levelName, const ActorSpawnDefinition& definition)
    {
        auto* level = FindStreamingLevel(levelName);
        if (level == nullptr)
        {
            auto createdName = shine::SString::from_view(levelName);
            level = &AddStreamingLevel(std::move(createdName), false);
        }

        level->actorDefinitions.push_back(definition);
        MarkDirty();
        return level->actorDefinitions.back();
    }

    bool MapAsset::RemoveActorFromPersistentLevel(shine::STextView actorName)
    {
        const bool removed = RemoveActorByName(persistentLevel_.actorDefinitions, actorName);
        if (removed)
        {
            MarkDirty();
        }

        return removed;
    }

    bool MapAsset::RemoveActorFromStreamingLevel(shine::STextView levelName, shine::STextView actorName)
    {
        auto* level = FindStreamingLevel(levelName);
        if (level == nullptr)
        {
            return false;
        }

        const bool removed = RemoveActorByName(level->actorDefinitions, actorName);
        if (removed)
        {
            MarkDirty();
        }

        return removed;
    }
}