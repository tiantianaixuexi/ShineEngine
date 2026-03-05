#pragma once

#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "EngineCore/asset/asset_base.h"
#include "gameplay/actor.h"
#include "math/vector.ixx"

namespace shine::gameplay::world
{
    struct WorldSettings
    {
        float gravityZ = -980.0f;
        float timeDilation = 1.0f;
        bool enableGlobalIllumination = true;
    };

    enum class ActorArchetype
    {
        EmptyActor,
        StaticMeshCube,
        StaticMeshSphere
    };

    struct ActorSpawnDefinition
    {
        std::string name;
        ActorArchetype archetype = ActorArchetype::EmptyActor;
        shine::math::FVector3f position{0.0f, 0.0f, 0.0f};
        shine::math::FVector3f scale{1.0f, 1.0f, 1.0f};
    };

    struct LevelAsset
    {
        enum class StreamingState
        {
            Unloaded,
            Loading,
            Loaded,
            Failed
        };

        std::string name;
        bool initiallyLoaded = false;
        StreamingState state = StreamingState::Unloaded;
        std::vector<ActorSpawnDefinition> actorDefinitions;
        std::vector<std::unique_ptr<shine::gameplay::SActor>> loadedActors;
        std::future<std::vector<ActorSpawnDefinition>> loadingTask;
    };

    class MapAsset : public shine::editor::asset::IAssetBase
    {
    public:
        explicit MapAsset(std::string mapName = "UntitledMap", std::string mapPath = "memory://map/untitled.map");

        const std::string& getName() const noexcept { return name_; }
        void setName(const std::string& name) { name_ = name; }

        WorldSettings& getWorldSettings() noexcept { return worldSettings_; }
        const WorldSettings& getWorldSettings() const noexcept { return worldSettings_; }

        LevelAsset& persistentLevel() noexcept { return persistentLevel_; }
        const LevelAsset& persistentLevel() const noexcept { return persistentLevel_; }

        std::vector<LevelAsset>& streamingLevels() noexcept { return streamingLevels_; }
        const std::vector<LevelAsset>& streamingLevels() const noexcept { return streamingLevels_; }

        LevelAsset* findStreamingLevel(std::string_view levelName) noexcept;
        const LevelAsset* findStreamingLevel(std::string_view levelName) const noexcept;
        LevelAsset& addStreamingLevel(const std::string& levelName);

    private:
        std::string name_;
        WorldSettings worldSettings_;
        LevelAsset persistentLevel_;
        std::vector<LevelAsset> streamingLevels_;
    };
}
