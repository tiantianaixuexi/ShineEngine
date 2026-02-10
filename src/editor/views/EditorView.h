#pragma once

#include "render/renderer_service.h"
#include <memory>

    // Forward declaration
    namespace shine::render::demo {
        class EngineDemoScene;
    }

namespace shine::editor::EditorView
{

    class EditView
    {
	public:

        EditView(shine::EngineContext& context);
        ~EditView();

        void Init();
        void Render(); // Note: Removed const to allow updating renderer state

    private:
        shine::EngineContext& m_Context;
        shine::render::ViewportHandle Viewport = 0;
        
        std::unique_ptr<shine::render::demo::EngineDemoScene> m_DemoScene;
    };

}
