#pragma once

#include "render/renderer_service.h"



namespace shine::editor::EditorView
{

	class EditView
	{
	public:

        void Init(render::RendererService* renderer);
		void Render();

    private:
        shine::render::RendererService* Renderer = nullptr;
        shine::render::ViewportHandle Viewport = 0;
	};

}

