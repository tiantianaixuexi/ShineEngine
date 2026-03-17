#include "MainEditorToolbar.h"

#include "imgui/imgui.h"
#include "editor/mainEditor.h"
#include "render/demo/EngineDemoScene.h"

namespace shine::editor::views
{
	SMainEditorToolbar::SMainEditorToolbar(main_editor::MainEditor* commands)
	{
		commands_ = commands;
	}

	void SMainEditorToolbar::onInit()
	{
	
	}

	void SMainEditorToolbar::onShutDown()
	{

	}
	
	void SMainEditorToolbar::onRender()
	{
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
			
			if (ImGui::BeginMenu("编辑器UI"))
			{
				if(ImGui::MenuItem("场景层级视口", nullptr, &SceneHierarchyShow))
				{
					SceneHierarchyShow = commands_ ? commands_->ToggleSceneHierarchy() : SceneHierarchyShow;
				}

				if (ImGui::MenuItem("资源浏览器",nullptr,&AssetBorderShow))
				{
					AssetBorderShow = commands_ ? commands_->ToggleAssetBrowser() : AssetBorderShow;
				}
				
                if (ImGui::MenuItem("内存监控 (Memory Profiler)", nullptr, &MemoryProfilerShow))
                {
					
                    MemoryProfilerShow = commands_ ? commands_->ToggleMemoryProfiler() : MemoryProfilerShow;
                }

				if(ImGui::MenuItem("引擎设置", nullptr, &EngineSettingsShow))
				{
					EngineSettingsShow = commands_ ? commands_->ToggleEngineSettings() : EngineSettingsShow;
				}

				if(ImGui::MenuItem("日志", nullptr, &LogShow))
				{
					LogShow = commands_ ? commands_->ToggleLog() : LogShow;
				}

				if(ImGui::MenuItem("物品放置栏", nullptr, &PlacementPaletteShow))
				{
					PlacementPaletteShow = commands_ ? commands_->TogglePlacementPalette() : PlacementPaletteShow;
				}

				if(ImGui::MenuItem("调试渲染Pass", nullptr, &DebugTextureShow))
				{
					DebugTextureShow = commands_ ? commands_->ToggleDebugTexture() : DebugTextureShow;
				}

				if(ImGui::MenuItem("属性大纲",nullptr,&PropertyInspectorShow))
				{
					PropertyInspectorShow = commands_ ? commands_->TogglePropertyInspector() : PropertyInspectorShow;
				}

				if(ImGui::MenuItem("Widget 编辑器",nullptr,&WidgetDesignerShow))
				{
					WidgetDesignerShow = commands_ ? commands_->ToggleWidgetDesigner() : WidgetDesignerShow;
				}

				ImGui::EndMenu();
			}
		
			
			ImGui::EndMainMenuBar();
		}
	}


}
