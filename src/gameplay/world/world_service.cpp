#include "world_service.h"

#include <algorithm>
#include <chrono>
#include <thread>

#include "EngineCore/engine_context.h"
#include "editor/ShineAsset/AssetMetadata.h"
#include "editor/ShineAsset/AssetTypes.h"
#include "editor/ShineAsset/AssetUuidHelper.h"
#include "editor/ShineAsset/EditorAssetRegistry.h"
#include "gameplay/actor.h"
#include "gameplay/object.h"
#include "gameplay/objectFlag.h"
#include "gameplay/component/StaticMeshComponent.h"
#include "gameplay/component/TransformComponent.h"
#include "gameplay/mesh/StaticMesh.h"


namespace shine::gameplay::world
{
    void WorldService::clearSelection()
    {
        for (uint32_t objectId : selectedObjectIds_)
        {
            if (const auto it = actorIndex_.find(objectId); it != actorIndex_.end() && it->second)
            {
                it->second->setFlag(shine::gameplay::EObjectFlags::OF_Selected, false);
            }
        }
        selectedObjectIds_.clear();
        selectedObjectId_ = 0;
    }

    void WorldService::setSelectedObject(shine::gameplay::SObject* obj)
    {
        clearSelection();
        addSelectionInternal(obj);
    }

    void WorldService::toggleSelectedObject(shine::gameplay::SObject* obj)
    {
        if (!obj)
        {
            return;
        }
        if (isSelected(obj))
        {
            removeSelectionInternal(obj);
            return;
        }
        addSelectionInternal(obj);
    }

    bool WorldService::isSelected(const shine::gameplay::SObject* obj) const
    {
        return obj && std::find(selectedObjectIds_.begin(), selectedObjectIds_.end(), obj->getObjectId()) != selectedObjectIds_.end();
    }

    shine::gameplay::SObject* WorldService::getSelectedObject() const
    {
        const auto it = actorIndex_.find(selectedObjectId_);
        return it == actorIndex_.end() ? nullptr : it->second;
    }

    std::vector<shine::gameplay::SObject*> WorldService::getSelectedObjectsSnapshot() const
    {
        std::vector<shine::gameplay::SObject*> selectedObjects;
        selectedObjects.reserve(selectedObjectIds_.size());
        for (uint32_t objectId : selectedObjectIds_)
        {
            const auto it = actorIndex_.find(objectId);
            if (it != actorIndex_.end() && it->second)
            {
                selectedObjects.push_back(it->second);
            }
        }
        return selectedObjects;
    }

    WorldService& WorldService::get()
    {
        return *EngineContext::Get().GetSystem<WorldService>();
    }

    bool WorldService::Init(EngineContext& ctx)
    {
        (void)ctx;
        clearSelection();
        actorIndex_.clear();
        return true;
    }

    void WorldService::Shutdown(EngineContext& ctx)
    {
        (void)ctx;
        clearSelection();
        activeMap_.reset();
        actorIndex_.clear();
    }

    void WorldService::createMapAsset(STextView mapName)
    {
        clearSelection();
        activeMap_ = std::make_unique<MapAsset>(SString(mapName));
        activeMapUuid_ = shine::editor::asset::GenerateUUIDString();
        rebuildActorIndex();
    }

    bool WorldService::saveMapAsset(const std::filesystem::path& sassetPath)
    {
        if (!activeMap_)
            return false;

        using namespace shine::editor::asset;

        // Generate UUID on first save
        if (activeMapUuid_.empty())
            activeMapUuid_ = GenerateUUIDString();

        // Build AssetMetadata
        AssetMetadata meta;
        meta.formatVersion = "2.0";
        auto& rec = meta.asset;
        rec.uuid     = activeMapUuid_.to_string();
        rec.type     = std::string(AssetTypeId::World);
        rec.imported = true;
        rec.lastImportTime = std::format("{:%FT%TZ}",
            std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));

        // Serialize world settings as import settings JSON
        const auto& ws = activeMap_->GetWorldSettings();
        struct SerializedWorldSettings
        {
            float gravityZ;
            float timeDilation;
            bool  enableGlobalIllumination;
        };

        SerializedWorldSettings sws{
            .gravityZ = ws.gravityZ,
            .timeDilation = ws.timeDilation,
            .enableGlobalIllumination = ws.enableGlobalIllumination
        };

        std::string wsJson = glz::write_json(sws);
        rec.importSettings = glz::raw_json{ std::move(wsJson) };

        // Write .sasset file
        auto writeResult = WriteAssetMetadataFile(meta, sassetPath.string());
        if (!writeResult)
            return false;

        // Register with EditorAssetRegistry if available
        if (EngineContext::IsInitialized())
        {
            auto* editorRegistry = EngineContext::Get().GetSystem<EditorAssetRegistry>();
            if (editorRegistry)
                editorRegistry->Register(sassetPath, std::move(rec));
        }

        return true;
    }

    bool WorldService::loadMapAsset(STextView uuid)
    {
        if (uuid.empty())
            return false;

        using namespace shine::editor::asset;

        if (!EngineContext::IsInitialized())
            return false;

        auto* editorRegistry = EngineContext::Get().GetSystem<EditorAssetRegistry>();
        if (!editorRegistry)
            return false;

        const auto* entry = editorRegistry->Find(uuid);
        if (!entry || entry->isDangling)
            return false;

        // Read the .sasset file
        auto result = ReadAssetMetadataFile(entry->diskPath.sv());
        if (!result)
            return false;

        // Reconstruct MapAsset from metadata
        const auto& rec = result->asset;
        SString mapName(rec.sourceFile.empty() ? "LoadedMap" : rec.sourceFile);

        clearSelection();
        activeMap_ = std::make_unique<MapAsset>(std::move(mapName));
        activeMapUuid_ = SString(rec.uuid);
        rebuildActorIndex();

        return true;
    }



    MapAsset* WorldService::getActiveMap() noexcept
    {
        return activeMap_.get();
    }

    const MapAsset* WorldService::getActiveMap() const noexcept
    {
        return activeMap_.get();
    }

    void WorldService::addActorToPersistentLevel(std::unique_ptr<shine::gameplay::SActor> actor)
    {
        if (!actor)
        {
            return;
        }
        auto* map = getActiveMap();
        if (!map)
        {
            createMapAsset("DefaultMap");
            map = getActiveMap();
        }
        if (!map)
        {
            return;
        }
        auto* actorPtr = actor.get();
        //map->persistentLevel().loadedActors.push_back(std::move(actor));
       /* if (actorPtr)
        {
            actorIndex_[actorPtr->getObjectId()] = actorPtr;
        }*/
    }

    bool WorldService::removeActor(shine::gameplay::SObject* actor)
    {
        auto* map = getActiveMap();
        if (!map || !actor)
        {
            return false;
        }

        if (getSelectedObject() == actor)
        {
            removeSelectionInternal(actor);
        }
        else if (isSelected(actor))
        {
            removeSelectionInternal(actor);
        }

     /*   auto eraseFrom = [actor](std::vector<std::unique_ptr<shine::gameplay::SActor>>& actors)
        {
            const auto oldSize = actors.size();
            actors.erase(
                std::remove_if(actors.begin(), actors.end(), [actor](const std::unique_ptr<shine::gameplay::SActor>& item)
                {
                    return item.get() == actor;
                }),
                actors.end());
            return actors.size() != oldSize;
        };

        if (eraseFrom(map->persistentLevel().loadedActors))
        {
            actorIndex_.erase(actor->getObjectId());
            return true;
        }

        for (auto& level : map->streamingLevels())
        {
            if (eraseFrom(level.loadedActors))
            {
                actorIndex_.erase(actor->getObjectId());
                return true;
            }
        }*/
        return false;
    }

    shine::gameplay::SObject* WorldService::findActorById(uint32_t objectId) const noexcept
    {
        const auto it = actorIndex_.find(objectId);
        if (it == actorIndex_.end())
        {
            return nullptr;
        }
        return it->second;
    }

    std::vector<shine::gameplay::SObject*> WorldService::getAllActorsSnapshot() const
    {
        std::vector<shine::gameplay::SObject*> snapshot;
        const auto* map = getActiveMap();
        if (!map)
        {
            return snapshot;
        }

        const auto appendActors = [&snapshot](const std::vector<std::unique_ptr<shine::gameplay::SActor>>& actors)
        {
            for (const auto& actor : actors)
            {
                if (actor)
                {
                //    snapshot.push_back(actor.get());
                }
            }
        };

      /*  appendActors(map->persistentLevel().loadedActors);
        for (const auto& level : map->streamingLevels())
        {
            if (level.state == LevelAsset::StreamingState::Loaded)
            {
                appendActors(level.loadedActors);
            }
        }*/
        return snapshot;
    }

   /* LevelAsset& WorldService::ensureStreamingLevel(const std::string& levelName)
    {
        auto* map = getActiveMap();
        if (!map)
        {
            createMapAsset("DefaultMap");
            map = getActiveMap();
        }
        static LevelAsset fallbackLevel;
        if (!map)
        {
            fallbackLevel.name = levelName;
            return fallbackLevel;
        }
        return map->addStreamingLevel(levelName);
    }*/

    bool WorldService::requestLoadLevelAsync(const std::string& levelName)
    {
        //auto& level = ensureStreamingLevel(levelName);
        //if (level.state == LevelAsset::StreamingState::Loaded || level.state == LevelAsset::StreamingState::Loading)
        //{
        //    return false;
        //}

        //level.state = LevelAsset::StreamingState::Loading;
        //const auto defs = level.actorDefinitions;
        //level.loadingTask = std::async(std::launch::async, [defs]()
        //{
        //    std::this_thread::sleep_for(std::chrono::milliseconds(1));
        //    return defs;
        //});
        return true;
    }

    bool WorldService::requestUnloadLevel(const std::string& levelName)
    {
        auto* map = getActiveMap();
        if (!map)
        {
            return false;
        }
        //auto* level = map->findStreamingLevel(levelName);
        //if (!level)
        //{
        //    return false;
        //}
        //level->loadedActors.clear();
        //level->state = LevelAsset::StreamingState::Unloaded;
        //rebuildActorIndex();
        //pruneSelection();
        return true;
    }

    void WorldService::tickStreaming()
    {
        auto* map = getActiveMap();
        if (!map)
        {
            return;
        }

     /*   for (auto& level : map->streamingLevels())
        {
            if (level.state != LevelAsset::StreamingState::Loading || !level.loadingTask.valid())
            {
                continue;
            }
            if (level.loadingTask.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
            {
                continue;
            }

            auto definitions = level.loadingTask.get();
            level.loadedActors.clear();
            for (const auto& def : definitions)
            {
                auto actor = instantiateActor(def);
                if (actor)
                {
                    level.loadedActors.push_back(std::move(actor));
                }
            }
            level.state = LevelAsset::StreamingState::Loaded;
            rebuildActorIndex();
            pruneSelection();
        }*/
    }

    void WorldService::rebuildActorIndex()
    {
        actorIndex_.clear();
        auto* map = getActiveMap();
        if (!map)
        {
            return;
        }

        /*indexActorVector(map->persistentLevel().loadedActors);
        for (const auto& level : map->streamingLevels())
        {
            if (level.state == LevelAsset::StreamingState::Loaded)
            {
                indexActorVector(level.loadedActors);
            }
        }*/
        pruneSelection();
    }

    void WorldService::indexActorVector(const std::vector<std::unique_ptr<shine::gameplay::SActor>>& actors)
    {
        for (const auto& actor : actors)
        {
            if (actor)
            {
                //actorIndex_[actor->getObjectId()] = actor.get();
            }
        }
    }

    void WorldService::addSelectionInternal(shine::gameplay::SObject* obj)
    {
        if (!obj)
        {
            selectedObjectId_ = 0;
            return;
        }
        if (isSelected(obj))
        {
            selectedObjectId_ = obj->getObjectId();
            return;
        }
        obj->setFlag(shine::gameplay::EObjectFlags::OF_Selected, true);
        selectedObjectIds_.push_back(obj->getObjectId());
        selectedObjectId_ = obj->getObjectId();
    }

    void WorldService::removeSelectionInternal(shine::gameplay::SObject* obj)
    {
        if (!obj)
        {
            return;
        }
        obj->setFlag(shine::gameplay::EObjectFlags::OF_Selected, false);
        selectedObjectIds_.erase(
            std::remove(selectedObjectIds_.begin(), selectedObjectIds_.end(), obj->getObjectId()),
            selectedObjectIds_.end());
        selectedObjectId_ = selectedObjectIds_.empty() ? 0 : selectedObjectIds_.back();
    }

    void WorldService::pruneSelection()
    {
        selectedObjectIds_.erase(
            std::remove_if(selectedObjectIds_.begin(), selectedObjectIds_.end(), [this](uint32_t objectId)
            {
                return !actorIndex_.contains(objectId);
            }),
            selectedObjectIds_.end());
        selectedObjectId_ = selectedObjectIds_.empty() ? 0 : selectedObjectIds_.back();
    }

    std::unique_ptr<shine::gameplay::SActor> WorldService::instantiateActor(const ActorSpawnDefinition& definition) const
    {
        std::unique_ptr<shine::gameplay::SActor> actor;
        if (definition.archetype == ActorArchetype::StaticMeshCube || definition.archetype == ActorArchetype::StaticMeshSphere)
        {
            auto meshActor = std::make_unique<shine::gameplay::StaticMeshActor>();
            auto* transform = meshActor->addComponent<shine::gameplay::component::TransformComponent>();
            transform->setPosition(definition.position);
            transform->setScale(definition.scale);

            auto* meshComp = meshActor->addComponent<shine::gameplay::component::StaticMeshComponent>();
            auto mesh = std::make_shared<shine::gameplay::StaticMesh>();
            if (definition.archetype == ActorArchetype::StaticMeshSphere)
            {
                mesh->initSphereWithNormals(24, 16);
            }
            else
            {
                mesh->initCubeWithNormals();
            }
            meshComp->setMesh(mesh);
            actor = std::move(meshActor);
        }
        else
        {
            auto emptyActor = std::make_unique<shine::gameplay::EmptyActor>();
            auto* transform = emptyActor->addComponent<shine::gameplay::component::TransformComponent>();
            transform->setPosition(definition.position);
            transform->setScale(definition.scale);
            actor = std::move(emptyActor);
        }

        if (actor)
        {
            actor->setName(definition.name);
        }
        return actor;
    }
}
