#pragma once

#include <vector>

#include "gameplay/actor.h"
#include "gameplay/object.h"
#include "gameplay/world/WorldServiceInterfaces.h"
#include "BaseView.h"

namespace shine::editor::views
{
    class SceneHierarchyView : public BaseView
    {
    public:
        
        virtual ~SceneHierarchyView(){};

        void SetWorldHierarchyService(shine::gameplay::world::IWorldActorHierarchyService* worldService);
        void onInit() override;
        void onShutDown() override;
        void onRender() override;

        void SetSelectedObject(shine::gameplay::SObject* obj);
        shine::gameplay::SObject* GetSelectedObject() const { return selectedObject_; }

    private:
        void refreshObjects();
        void RenderObjectNode(shine::gameplay::SObject* obj, int index);
        void createEmptyActor();
        void createStaticMeshActor();
        void deleteObject(shine::gameplay::SObject* obj);
        bool isEditorOwned(const shine::gameplay::SObject* obj) const;

        shine::gameplay::world::IWorldActorHierarchyService* worldService_ = nullptr;
        std::vector<shine::gameplay::SObject*> visibleObjects_;
        shine::gameplay::SObject* selectedObject_ = nullptr;
        shine::gameplay::SObject* contextObject_ = nullptr;
        char searchBuffer_[128]{};
        char renameBuffer_[128]{};
        int nextEmptyActorId_ = 1;
        int nextStaticMeshActorId_ = 1;

    };
}

