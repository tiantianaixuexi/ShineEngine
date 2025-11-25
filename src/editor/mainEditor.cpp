#include "mainEditor.h"

#include <cstdint>
#include <memory>

#include "fmt/format.h"
#include "imgui.h"
namespace shine::editor::main_editor {

	MainEditor::MainEditor()
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

	void MainEditor::Init(render::backend::IRenderBackend* render) {

		RenderBackend = render;

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

		assetsBrower = new editor::assets_brower::AssetsBrower();
		assetsBrower->Start();

        // 初始化渲染服务（单实例封装）
        Renderer = &render::RendererService::get();
        Renderer->init(RenderBackend);
        editorView = new EditorView::EditView();
        editorView->Init(Renderer);

		// 初始化场景层级视图
		sceneHierarchyView = new views::SceneHierarchyView();

		// 初始化属性面板
		propertiesView = new views::PropertiesView();

		// 初始化图片查看器（不再需要 Init，直接使用即可）
		imageViewerView = new views::ImageViewerView();
	}

	void MainEditor::Render() {

		ImGuiDockNodeFlags mainDockNodeFlags = ImGuiDockNodeFlags_None;
		ImGuiWindowFlags window_flags =
			ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		const ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |=
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::Begin("DockSpace Demo", &mainDocker, window_flags);
		ImGui::PopStyleVar(2);
		ImGuiID MainDock_id = ImGui::GetID("Engine Main Dock");
		ImGui::DockSpace(MainDock_id, ImVec2(0.0f, 0.0f), mainDockNodeFlags);
		ImGui::End();

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				ImGui::MenuItem("(demo menu)", nullptr, false, false);
				if (ImGui::MenuItem("New")) {
				}
				if (ImGui::MenuItem("Open", "Ctrl+O")) {
				}
				if (ImGui::BeginMenu("Open Recent")) {
					ImGui::MenuItem("fish_hat.c");
					ImGui::MenuItem("fish_hat.inl");
					ImGui::MenuItem("fish_hat.h");
					if (ImGui::BeginMenu("More..")) {
						ImGui::MenuItem("Hello");
						ImGui::MenuItem("Sailor");
						if (ImGui::BeginMenu("Recurse..")) {
							ImGui::EndMenu();
						}
						ImGui::EndMenu();
					}
					ImGui::EndMenu();
				}
				if (ImGui::MenuItem("Save", "Ctrl+S")) {
				}
				if (ImGui::MenuItem("Save As..")) {
				}

				ImGui::Separator();
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit")) {
				if (ImGui::MenuItem("Undo", "CTRL+Z")) {
				}
				if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {
				} // Disabled item
				ImGui::Separator();
				if (ImGui::MenuItem("Cut", "CTRL+X")) {
				}
				if (ImGui::MenuItem("Copy", "CTRL+C")) {
				}
				if (ImGui::MenuItem("Paste", "CTRL+V")) {
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

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
} // namespace shine::editor::main_editor


