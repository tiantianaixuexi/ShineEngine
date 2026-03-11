#pragma once

#include <cstdint>

#include "EngineCore/asset/BaseAsset.h"
#include "math/vector.ixx"
#include "string/shine_string.h"
#include "string/shine_text_view.h"

namespace shine::gameplay::world
{
    struct WorldSettings
    {
        float gravityZ = -980.0f;
        float timeDilation = 1.0f;
        bool enableGlobalIllumination = true;
    };

    enum class ActorArchetype : std::uint8_t
    {
        EmptyActor = 0,
        StaticMeshCube,
        StaticMeshSphere
    };

    struct ActorSpawnDefinition
    {
        shine::SString name;
        ActorArchetype archetype = ActorArchetype::EmptyActor;
        shine::math::FVector3f position{0.0f, 0.0f, 0.0f};
        shine::math::FVector3f rotation{0.0f, 0.0f, 0.0f};
        shine::math::FVector3f scale{1.0f, 1.0f, 1.0f};
    };

    struct LevelDefinition
    {
        shine::SString name;
        bool initiallyLoaded = false;
        std::vector<ActorSpawnDefinition> actorDefinitions;
    };

    class MapAsset final : public shine::editor::asset::AssetBase
    {
    public:
        MapAsset();
        explicit MapAsset(
            shine::SString mapName,
            const shine::SString& logicalPath = shine::SString("/game/maps/untitled"));

        [[nodiscard]] shine::STextView GetClassName() const noexcept override
        {
            return shine::STextView::from_literal("MapAsset");
        }

        [[nodiscard]] WorldSettings& GetWorldSettings() noexcept;
        [[nodiscard]] const WorldSettings& GetWorldSettings() const noexcept;

        void SetWorldSettings(const WorldSettings& settings);
        void SetGravityZ(float gravityZ);
        void SetTimeDilation(float timeDilation);
        void SetGlobalIlluminationEnabled(bool enabled);

        [[nodiscard]] LevelDefinition& GetPersistentLevel() noexcept;
        [[nodiscard]] const LevelDefinition& GetPersistentLevel() const noexcept;

        [[nodiscard]] std::vector<LevelDefinition>& GetStreamingLevels() noexcept;
        [[nodiscard]] const std::vector<LevelDefinition>& GetStreamingLevels() const noexcept;

        [[nodiscard]] LevelDefinition* FindStreamingLevel(shine::STextView levelName) noexcept;
        [[nodiscard]] const LevelDefinition* FindStreamingLevel(shine::STextView levelName) const noexcept;

        LevelDefinition& AddStreamingLevel(shine::SString levelName, bool initiallyLoaded = false);
        bool RemoveStreamingLevel(shine::STextView levelName);
        void ClearStreamingLevels();

        ActorSpawnDefinition& AddActorToPersistentLevel(const ActorSpawnDefinition& definition);
        ActorSpawnDefinition& AddActorToStreamingLevel(shine::STextView levelName, const ActorSpawnDefinition& definition);

        bool RemoveActorFromPersistentLevel(shine::STextView actorName);
        bool RemoveActorFromStreamingLevel(shine::STextView levelName, shine::STextView actorName);

    private:
        static LevelDefinition CreatePersistentLevelDefinition();
        static bool RemoveActorByName(std::vector<ActorSpawnDefinition>& definitions, shine::STextView actorName);

    private:
        WorldSettings worldSettings_{};
        LevelDefinition persistentLevel_{};
        std::vector<LevelDefinition> streamingLevels_{};
    };
}