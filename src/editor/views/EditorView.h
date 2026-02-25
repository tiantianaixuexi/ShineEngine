#pragma once

#include "render/renderer_service.h"
#include "BaseView.h"
#include <memory>

    // Forward declaration
    namespace shine::render::demo {
        class EngineDemoScene;
    }

namespace shine::editor::views
{

    class EditView : public BaseView
    {
	public:


        virtual ~EditView();

        void onInit()    override;
        void onRender()  override; // Note: Removed const to allow updating renderer state
        void onShutDown() override;
    private:

        shine::render::ViewportHandle Viewport = 0;
        
        std::unique_ptr<shine::render::demo::EngineDemoScene> m_DemoScene;
    };

}
