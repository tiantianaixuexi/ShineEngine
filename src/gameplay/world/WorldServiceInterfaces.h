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

        virtual void clearSelection() = 0;
        virtual void setSelectedObject(shine::gameplay::SObject* obj) = 0;
        virtual void toggleSelectedObject(shine::gameplay::SObject* obj) = 0;
        virtual shine::gameplay::SObject* getSelectedObject() const = 0;
        virtual std::vector<shine::gameplay::SObject*> getSelectedObjectsSnapshot() const = 0;
        virtual bool isSelected(const shine::gameplay::SObject* obj) const = 0;
    };
}
