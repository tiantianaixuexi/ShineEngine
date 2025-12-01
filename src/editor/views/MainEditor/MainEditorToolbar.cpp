#include "MainEditorToolbar.h"

#include "imgui.h"
#include "editor/mainEditor.h"


namespace shine::editor::views
{
	SMainEditorToolbar::SMainEditorToolbar(main_editor::MainEditor* _editor)
	{
		_mainEditor = _editor;
	}

	void SMainEditorToolbar::Init()
	{
	
	}

	void SMainEditorToolbar::Render()
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
				static bool AssetBorderShow = true;
				if (ImGui::MenuItem("资源浏览器",nullptr,&AssetBorderShow))
				{
					AssetBorderShow = _mainEditor->setAssetBorwerOpen();
				}
				

				ImGui::EndMenu();
			}


			ImGui::EndMainMenuBar();
		}
	}


}
