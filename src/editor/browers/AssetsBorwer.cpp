#include "AssetsBrower.h"


#include <string>
#include <locale>


namespace shine::editor::assets_brower {

void AssetsBrower::Start()
{
    //AddItem();
}

void AssetsBrower::Render() {

    if (!isOpen) return;
    
    if (ImGui::Begin(title.c_str(), &isOpen))
    {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                ImGui::Separator();
            }
            ImGui::EndMenu();

            ImGui::EndMenuBar();
        }

        static int Selected = 0;
        {
            ImGui::BeginChild("Asset Left", ImVec2(150, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
            for (int i = 0; i < 100; i++)
            {

                std::string sss("Objcet ");
                sss.append(std::to_string(i));
                if (ImGui::Selectable(sss.c_str(), Selected == i))
                {
                    Selected = i;
                }
            }
            ImGui::EndChild();
        }

        ImGuiIO& io = ImGui::GetIO();

        // ImGui::SetNextWindowContentSize(ImVec2(0.f,))
        if (ImGui::BeginChild("##Assets",
            ImVec2(0.f, -ImGui::GetTextLineHeightWithSpacing()),
            ImGuiChildFlags_Borders, ImGuiWindowFlags_NoMove)) {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            const float avail_width = ImGui::GetContentRegionAvail().x;

            ImGuiMultiSelectFlags ms_flags = ImGuiMultiSelectFlags_ClearOnEscape |
                ImGuiMultiSelectFlags_ClearOnClickVoid;

            ms_flags |= ImGuiMultiSelectFlags_BoxSelect2d;

            // ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(ms_flags,)
        }


        ImGui::EndChild();
    }

    ImGui::End();
    
}

bool AssetsBrower::SetShow() noexcept
{
	return (isOpen == true ? isOpen = false : isOpen = true);
}

void AssetsBrower::AddItem() {
  if (items.Size == 0) {
    NextItemId = 1;
  }

  items.reserve(items.Size + 100);
  for (int i = 0; i < 100; i++, NextItemId++) {
    items.push_back(
        assets_item::AssetsItem(NextItemId, (NextItemId % 20) < 15   ? 0
                                            : (NextItemId % 20) < 18 ? 1
                                                                     : 2));
  }
  RequestSort = true;
}



} // namespace shine::editor::assets_brower


