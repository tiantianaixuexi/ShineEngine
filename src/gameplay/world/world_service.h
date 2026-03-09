#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "EngineCore/subsystem.h"
#include "manager/AssetInterfaces.h"
#include "map_asset.h"
#include "WorldServiceInterfaces.h"

namespace shine::gameplay::world
{
    class WorldService : public shine::Subsystem, public IWorldActorHierarchyService
    {
    public:
        static WorldService& get();

        bool Init(EngineContext& ctx) override;
        void Shutdown(EngineContext& ctx) override;

        void createMapAsset(const std::string& mapName);
        bool activateMapAsset(const manager::AssetHandle& mapHandle);
        bool hasMap() const noexcept { return activeMapHandle_.isValid(); }

        MapAsset* getActiveMap() noexcept;
        const MapAsset* getActiveMap() const noexcept;

        void addActorToPersistentLevel(std::unique_ptr<shine::gameplay::SActor> actor) override;
        bool removeActor(shine::gameplay::SObject* actor);
        std::vector<shine::gameplay::SObject*> getAllActorsSnapshot() const;
        shine::gameplay::SObject* findActorById(uint32_t objectId) const noexcept;

        void clearSelection() override;
        void setSelectedObject(shine::gameplay::SObject* obj) override;
        void toggleSelectedObject(shine::gameplay::SObject* obj) override;
        shine::gameplay::SObject* getSelectedObject() const override;
        std::vector<shine::gameplay::SObject*> getSelectedObjectsSnapshot() const override;
        bool isSelected(const shine::gameplay::SObject* obj) const override;

        LevelAsset& ensureStreamingLevel(const std::string& levelName);
        bool requestLoadLevelAsync(const std::string& levelName);
        bool requestUnloadLevel(const std::string& levelName);
        void tickStreaming();

    private:
        std::unique_ptr<shine::gameplay::SActor> instantiateActor(const ActorSpawnDefinition& definition) const;
        void rebuildActorIndex();
        void indexActorVector(const std::vector<std::unique_ptr<shine::gameplay::SActor>>& actors);
        void addSelectionInternal(shine::gameplay::SObject* obj);
        void removeSelectionInternal(shine::gameplay::SObject* obj);
        void pruneSelection();

    private:
        manager::IWorldAssetBridge* worldAssetBridge_ = nullptr;
        manager::AssetHandle activeMapHandle_;
        std::unordered_map<uint32_t, shine::gameplay::SObject*> actorIndex_;
        uint32_t selectedObjectId_ = 0;
        std::vector<uint32_t> selectedObjectIds_;
    };
}
