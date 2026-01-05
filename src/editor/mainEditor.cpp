#include "mainEditor.h"

#include <cstdint>
#include <memory>

#include "fmt/format.h"
#include "imgui.h"

#include "editor/views/SceneHierarchyView.h"
#include "editor/views/PropertiesView.h"
#include "editor/views/ImageViewerView.h"
#include "views/MainEditor/MainEditorToolbar.h"

namespace shine::editor::main_editor {

	MainEditor::MainEditor(shine::EngineContext& context) : m_Context(context)
	{
	}

	MainEditor::~MainEditor()
	{
		delete myButton;
		delete assetsBrower;
		delete editorView;
		delete sceneHierarchyView;
		delete propertiesView;
		delete imageViewerView;
	}

	void MainEditor::Init() {


		myButton = new widget::button::shineButton("应用编辑");

		myButton->SetOnPressed([]() {
			fmt::println("应用编辑按钮被按下");
			// 在这里添加按钮按下后的编辑
			});

		myButton->SetOnReleased([]() {
			fmt::println("应用编辑按钮被释放");
			// 在这里添加按钮释放后的编辑
			});

		myButton->SetOnHovered([]() {
			fmt::println("应用编辑按钮被悬停");
			// 在这里添加按钮悬停后的编辑
			});

		myButton->SetOnUnHovered([]() { fmt::println("应用编辑按钮停止"); });


		mainEditorToolbar = new views::SMainEditorToolbar(this);

		
		assetsBrower = new assets_brower::AssetsBrower();
		assetsBrower->Start();


        // 初始化渲染服务（单实例封装）
        editorView = new EditorView::EditView(m_Context);
        editorView->Init();

		// 初始化场景层级视图
		sceneHierarchyView = new views::SceneHierarchyView();

		// 初始化属性面板
		propertiesView = new views::PropertiesView();

		// 初始化图片查看器（不再需要 Init，直接使用即可）
		imageViewerView = new views::ImageViewerView();
	}


	static ImGuiDockNodeFlags mainDockNodeFlags = ImGuiDockNodeFlags_None;
	static ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	static bool isDragging = false;
	void MainEditor::Render() {

		static bool showWindows = true;
		ImGui::ShowDemoWindow(&showWindows);

		const ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		ImGui::Begin("DockSpace Demo", &mainDocker, window_flags);
		ImGui::PopStyleVar(2);
		ImGuiID MainDock_id = ImGui::GetID("Engine Main Dock");
		ImGui::DockSpace(MainDock_id, ImVec2(0.0f, 0.0f), mainDockNodeFlags);
		ImGui::End();

		
		mainEditorToolbar->Render();
		assetsBrower->Render();

		// 编辑视图由 EditorView 渲染
        editorView->Render();

		// 渲染场景层级视图
		if (sceneHierarchyView)
		{
			sceneHierarchyView->Render();
			// 同步选中对象到属性面板
			if (propertiesView)
			{
				propertiesView->SetSelectedObject(sceneHierarchyView->GetSelectedObject());
			}
		}

		// 渲染属性面板
		if (propertiesView)
		{
			propertiesView->Render();
		}

		// 渲染图片查看器
		if (imageViewerView)
		{
			imageViewerView->Render();
		}

		// 渲染ImGui界面
		ImGui::Render();
	}

	bool MainEditor::setAssetBorwerOpen()
	{
		return assetsBrower->SetShow();
	}
} // namespace shine::editor::main_editor
