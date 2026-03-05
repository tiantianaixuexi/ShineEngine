#pragma once

#include <cstdint>
#include <memory>

#include "editor/views/placement/PlacementPayload.h"
#include "render/renderer_service.h"
#include "BaseView.h"

namespace shine::render::demo
{
    class EngineDemoScene;
}

namespace shine::gameplay
{
    class Camera;
}

namespace shine::gameplay::world
{
    class IWorldActorPlacementService;
}

namespace shine::editor::views
{

    class EditView : public BaseView
    {
	public:


        virtual ~EditView();

        void SetWorldPlacementService(shine::gameplay::world::IWorldActorPlacementService* worldPlacementService);
        void onInit()    override;
        void onRender()  override; // Note: Removed const to allow updating renderer state
        void onShutDown() override;

    private:
        void SpawnPlacementActor(EPlacementItemType type, float scale, gameplay::Camera* cam);

    private:

        shine::render::ViewportHandle Viewport = 0;
        uint64_t nextPlacedActorId_ = 1;
        shine::gameplay::world::IWorldActorPlacementService* worldPlacementService_ = nullptr;

        std::unique_ptr<shine::render::demo::EngineDemoScene> m_DemoScene;
    };

}
