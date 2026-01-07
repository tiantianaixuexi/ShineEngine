#pragma once

#include "widget/shineButton.h"
#include "editor/browers/AssetsBrower.h"
#include "editor/views/EditorView.h"

// 前向声明
namespace shine::editor::views
{
    class SceneHierarchyView;
    class PropertiesView;
    class ImageViewerView;
    class SettingsView;
	class SMainEditorToolbar;
}

namespace shine::editor::main_editor
{
	using namespace widget;

	 class MainEditor{

	public:

		MainEditor(shine::EngineContext& context);
		~MainEditor();

        void Init();
		void Render();


		bool mainDocker = true;



		bool setAssetBorwerOpen();

	 private:
        shine::EngineContext& m_Context;

		assets_brower::AssetsBrower* assetsBrower = nullptr;
        EditorView::EditView* editorView = nullptr;

		// 视图窗口
		views::SMainEditorToolbar* mainEditorToolbar = nullptr;
		views::SceneHierarchyView* sceneHierarchyView = nullptr;
		views::PropertiesView* propertiesView = nullptr;
		views::ImageViewerView* imageViewerView = nullptr;
        views::SettingsView* settingsView = nullptr;

		button::shineButton* myButton = nullptr;

	};

}
