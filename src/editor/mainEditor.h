#pragma once

#include "widget/shineButton.h"
#include "editor/browers/AssetsBrower.h"
#include "render/render_backend.h"
#include "editor/views/EditorView.h"

namespace shine::editor::main_editor
{
	using namespace widget;

	 class MainEditor{

	public:

		MainEditor();

        void Init(render::backend::IRenderBackend* render);
		void Render();




		bool mainDocker = true;


        render::backend::IRenderBackend* RenderBackend = nullptr;
        render::RendererService* Renderer = nullptr;


		assets_brower::AssetsBrower* assetsBrower = nullptr;
        EditorView::EditView* editorView = nullptr;



		button::shineButton* myButton = nullptr;

	};

}

