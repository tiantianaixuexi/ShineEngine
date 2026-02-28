#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"


namespace shine::widget
{
bool IconTreeNode(const char *label, ImTextureID openId, ImTextureID closeId, ImGuiTreeNodeFlags flags)
    {
        ImGuiContext& g = *ImGui::GetCurrentContext();
        ImGuiWindow* window = g.CurrentWindow;

        ImGuiID id = window->GetID(label);
        ImVec2 pos = window->DC.CursorPos;
        ImRect bb(pos, ImVec2(pos.x + ImGui::GetContentRegionAvail().x, pos.y + g.FontSize + g.Style.FramePadding.y*2));
        bool opened = ImGui::TreeNodeUpdateNextOpen(id, flags);
        bool hovered, held;
        if (ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick))
            window->DC.StateStorage->SetInt(id, opened ? 0 : 1);
        if (hovered || held)
            window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(held ? ImGuiCol_HeaderActive : ImGuiCol_HeaderHovered));

        // Icon, text
        const ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
        float button_sz = g.FontSize + g.Style.FramePadding.y*2;
        const float  button_se_2 = button_sz * 2.0f;

        ImGui::RenderArrow(window->DrawList,ImVec2(pos .x + g.Style.ItemInnerSpacing.x ,  pos.y+ g.Style.FramePadding.y), text_col,opened ? ImGuiDir_Down : ImGuiDir_Right, 0.8f);

        ImGui::SetCursorPos(ImVec2(pos.x + g.Style.ItemInnerSpacing.x * 2 , button_sz + g.Style.ItemInnerSpacing.y));
        ImGui::PushID("89848");
        ImGui::Image(opened ? openId : closeId, ImVec2(24,24));
        ImGui::PopID();
        ImGui::RenderText(ImVec2(pos.x + button_se_2 + g.Style.ItemInnerSpacing.x, pos.y + g.Style.FramePadding.y / 2), label);

        ImGui::SetCursorPos(ImVec2(pos.x, button_sz + g.Style.ItemInnerSpacing.y));


        ImGui::ItemSize(bb, g.Style.FramePadding.y);
        ImGui::ItemAdd(bb, id);

        if (opened)
            ImGui::TreePush(label);
        return opened;
    }
}