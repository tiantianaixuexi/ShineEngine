#include "AssetsBrower.h"

#include <locale>
#include <string>

#include "util/image_util.h"
#include "widget/CustomNode/IconTreeNode.h"
#include <fmt/base.h>

namespace shine::editor::assets_brower {

util::Image *win_close = nullptr;
util::Image *win_open  = nullptr;

#define Render_Floder_Tree_Node(name) \
    if (widget::IconTreeNode(name, win_open->textureId, win_close->textureId, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DrawLinesFull))

void AssetsBrower::onInit() {

    auto win_close_result = util::load_image_with_texture("E:\\c++\\ShineEngine\\Content\\icon\\icon_win_close.png");
    if (win_close_result.has_value()) {
        win_close = *win_close_result;
    } else {
        fmt::println("加载失败:{}", win_close_result.error());
    }
    auto win_open_result = util::load_image_with_texture("E:\\c++\\ShineEngine\\Content\\icon\\icon_win_open.png");
    if (win_open_result.has_value()) {
        win_open = *win_open_result;
    } else {
        fmt::println("加载失败:{}", win_open_result.error());
    }

    // 获取资产目录
}

void AssetsBrower::onRender() {

    if (ImGui::Begin(title.c_str(), &isOpen)) {
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
            if (ImGui::TreeNodeEx("所有", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DrawLinesFull)) {

                Render_Floder_Tree_Node("场景") {
                    for (int i = 0; i < 100; i++) {

                        std::string sss("Objcet ");
                        sss.append(std::to_string(i));
                        if (ImGui::Selectable(sss.c_str(), Selected == i)) {
                            Selected = i;
                        }
                    }
                    ImGui::TreePop();
                }

                Render_Floder_Tree_Node("内容") {

                    for (int i = 0; i < 100; i++) {

                        std::string sss("Objcet ");
                        sss.append(std::to_string(i));
                        if (ImGui::Selectable(sss.c_str(), Selected == i)) {
                            Selected = i;
                        }
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::PopID();

            ImGui::EndChild();
        }

        // ImGui::SetNextWindowContentSize(ImVec2(0.f,))
        if (ImGui::BeginChild("##Assets",
                              ImVec2(0.f, -ImGui::GetTextLineHeightWithSpacing()),
                              ImGuiChildFlags_Borders, ImGuiWindowFlags_NoMove)) {

            ImGuiMultiSelectFlags ms_flags = ImGuiMultiSelectFlags_ClearOnEscape |
                                             ImGuiMultiSelectFlags_ClearOnClickVoid;

            ms_flags |= ImGuiMultiSelectFlags_BoxSelect2d;

            // ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(ms_flags,)
        }

        ImGui::EndChild();
    }

    ImGui::End();

    CheckIsOpenChange();
}

void AssetsBrower::onShutDown() {
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
