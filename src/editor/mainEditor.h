#pragma once

#include "widget/shineButton.h"
#include "editor/browers/AssetsBrower.h"
#include "render/core/render_backend.h"
#include "editor/views/EditorView.h"

// 前向声明
namespace shine::editor::views
{
    class SceneHierarchyView;
    class PropertiesView;
    class ImageViewerView;
}

namespace shine::editor::main_editor
{
	using namespace widget;

	 class MainEditor{

	public:

		MainEditor();
		~MainEditor();

        void Init(render::backend::IRenderBackend* render);
		void Render();




		bool mainDocker = true;


        render::backend::IRenderBackend* RenderBackend = nullptr;
        render::RendererService* Renderer = nullptr;


		assets_brower::AssetsBrower* assetsBrower = nullptr;
        EditorView::EditView* editorView = nullptr;

		// 视图窗口
		views::SceneHierarchyView* sceneHierarchyView = nullptr;
		views::PropertiesView* propertiesView = nullptr;
		views::ImageViewerView* imageViewerView = nullptr;

		button::shineButton* myButton = nullptr;

	};

}

