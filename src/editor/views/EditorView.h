#pragma once

#include "render/renderer_service.h"

namespace shine::editor::EditorView
{

    class EditView
    {
	public:

        EditView(shine::EngineContext& context) : m_Context(context) {}

        void Init();
        void Render() const;

    private:
        shine::EngineContext& m_Context;
        shine::render::ViewportHandle Viewport = 0;
    };

}
