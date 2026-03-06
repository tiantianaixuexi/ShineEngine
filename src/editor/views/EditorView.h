#pragma once

#include <cstdint>
#include <memory>

#include "editor/views/placement/PlacementPayload.h"
#include "render/renderer_service.h"
#include "BaseView.h"

struct ImVec2;

namespace shine::render::demo
{
    class EngineDemoScene;
}

namespace shine::gameplay
{
    class Camera;
    class SObject;
}

namespace shine::gameplay::world
{
    class IWorldActorPlacementService;
    class IWorldActorHierarchyService;
}

namespace shine::editor::views
{

    class EditView : public BaseView
    {
	public:


        virtual ~EditView();

        void SetWorldPlacementService(shine::gameplay::world::IWorldActorPlacementService* worldPlacementService);
        void SetWorldHierarchyService(shine::gameplay::world::IWorldActorHierarchyService* worldHierarchyService);
        void SetSelectedObject(shine::gameplay::SObject* obj);
        [[nodiscard]] shine::gameplay::SObject* GetSelectedObject() const { return selectedObject_; }
        void onInit()    override;
        void onRender()  override; // Note: Removed const to allow updating renderer state
        void onShutDown() override;

    private:
        shine::gameplay::SObject* PickObjectInViewport(
            const ImVec2& viewportMin,
            const ImVec2& viewportSize,
            const ImVec2& mousePos,
            gameplay::Camera* cam
        ) const;
        void DrawSelectedObjectOutline(const ImVec2& viewportMin, const ImVec2& viewportSize, gameplay::Camera* cam) const;
        void SpawnPlacementActor(EPlacementItemType type, float scale, gameplay::Camera* cam);

    private:

        shine::render::ViewportHandle Viewport = 0;
        uint64_t nextPlacedActorId_ = 1;
        shine::gameplay::world::IWorldActorPlacementService* worldPlacementService_ = nullptr;
        shine::gameplay::world::IWorldActorHierarchyService* worldHierarchyService_ = nullptr;
        shine::gameplay::SObject* selectedObject_ = nullptr;

        std::unique_ptr<shine::render::demo::EngineDemoScene> m_DemoScene;
    };

}
