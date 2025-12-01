#pragma once

#include "render/renderer_service.h"



namespace shine::editor::EditorView
{

	class EditView
	{
	public:

        void Init();
		void Render();

    private:

        shine::render::ViewportHandle Viewport = 0;
	};

}

