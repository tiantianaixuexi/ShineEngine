#include "mainEditor.h"

#include <cstdint>
#include <memory>

#include "fmt/format.h"
#include "imgui/imgui.h"

#include "editor/views/EditorView.h"
#include "editor/views/SceneHierarchyView.h"
#include "editor/views/PropertiesView.h"
#include "editor/views/ImageViewerView.h"
#include "editor/views/SettingsView.h"
#include "editor/views/Profiler/MemoryProfiler.h"
#include "views/MainEditor/MainEditorToolbar.h"
#include "render/demo/EngineDemoScene.h"

namespace shine::editor::main_editor {

	REGISTER_LOG_GROUP_END(EditorLog)

	MainEditor::MainEditor(shine::EngineContext& context) : m_Context(context)
	{
		ADD_LOG_CATEGORYS(EditorLog, "init", "Rendering", "Input", "Assets", "Memory")

		SHINE_LOG_INFO(EditorLog, "init", "MainEditor constructor called");
	}

	MainEditor::~MainEditor()
	{
		delete myButton;
		delete assetsBrower;
		delete editorView;
		delete sceneHierarchyView;
		delete propertiesView;
		delete imageViewerView;
        delete settingsView;
        delete memoryProfiler;
        delete logUI;
	}

	void MainEditor::Init() {
		SHINE_LOG_INFO(EditorLog, "init", "[MainEditor] Init Start");

		mainEditorToolbar = new views::SMainEditorToolbar(this);
		mainEditorToolbar->SetShow();



		myButton = new widget::button::shineButton("应用编辑");

		myButton->SetOnPressed([]() {
			fmt::println("应用编辑按钮被按下");
			});

		myButton->SetOnReleased([]() {
			fmt::println("应用编辑按钮被释放");
			});

		myButton->SetOnHovered([]() {
			fmt::println("应用编辑按钮被悬停");
			});

		myButton->SetOnUnHovered([]() { fmt::println("应用编辑按钮停止"); });


        // 提前初始化内存监控，以便尽早可用
        memoryProfiler = new views::MemoryProfiler();

		
		assetsBrower = new assets_brower::AssetsBrower();
		assetsBrower->onInit();


        // 初始化渲染服务（单实例封装）
        editorView = new views::EditView();
        editorView->onInit();
		editorView->SetShow();

		// 初始化场景层级视图
		sceneHierarchyView = new views::SceneHierarchyView();
		sceneHierarchyView->onInit();

		// 初始化属性面板
		propertiesView = new views::PropertiesView();
		propertiesView->onInit();

		// 初始化图片查看器（不再需要 Init，直接使用即可）
		imageViewerView = new views::ImageViewerView();

        // 初始化引擎设置视图
        settingsView = new views::SettingsView();
		settingsView->OnOpenChange.bind([this](bool isOpen) {
			mainEditorToolbar->EngineSettingsShow = isOpen;
		});
		settingsView->onInit();

		
        // 初始化日志 UI
        logUI = new views::LogUI();
		logUI->onInit();


		SHINE_LOG_INFO(EditorLog, "init", "[MainEditor] Init Finish");
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

		
		mainEditorToolbar->RenderBase();
		assetsBrower->RenderBase();


        editorView->RenderBase();

		// 渲染场景层级视图

		sceneHierarchyView->RenderBase();
		propertiesView->SetSelectedObject(sceneHierarchyView->GetSelectedObject());

		propertiesView->RenderBase();
		imageViewerView->Render();
        settingsView->RenderBase();
        memoryProfiler->RenderBase();
        logUI->RenderBase();

		ImGui::Render();
	}

	bool MainEditor::setAssetBorwerOpen()
	{
		return assetsBrower->SetShow();
	}

    bool MainEditor::setMemoryProfilerOpen()
    {
        return memoryProfiler->SetShow();
    }

	bool MainEditor::setEngineSettingsOpen()
	{
		return settingsView->SetShow();
	}

    bool MainEditor::getMemoryProfilerOpen() const
    {
        return memoryProfiler ? memoryProfiler->IsOpen() : false;
    }
} // namespace shine::editor::main_editor
