#pragma once

#include <memory>
#include <vector>

namespace shine::gameplay
{
    class SActor;
    class SObject;
}

namespace shine::gameplay::world
{
    class IWorldActorPlacementService
    {
    public:
        virtual ~IWorldActorPlacementService() = default;
        virtual void addActorToPersistentLevel(std::unique_ptr<shine::gameplay::SActor> actor) = 0;
    };

    class IWorldActorHierarchyService : public IWorldActorPlacementService
    {
    public:
        virtual ~IWorldActorHierarchyService() = default;
        virtual bool removeActor(shine::gameplay::SObject* actor) = 0;
        virtual std::vector<shine::gameplay::SObject*> getAllActorsSnapshot() const = 0;
    };
}
