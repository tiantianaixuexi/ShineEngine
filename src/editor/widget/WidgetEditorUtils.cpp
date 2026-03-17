#include "WidgetEditorUtils.h"

namespace shine::editor::widget::utils {

ImVec2 GetLocalCursor()
{
    ImGuiIO&      io         = ImGui::GetIO();
    ImGuiContext& g          = *ImGui::GetCurrentContext();
    ImGuiWindow*  w          = g.CurrentWindow;
    ImVec2        cursor     = ImVec2(io.MousePos.x - w->Pos.x, io.MousePos.y - w->Pos.y);
    ImRect        itemrect   = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    float         itemwidth  = itemrect.Max.x - itemrect.Min.x;
    float         itemheight = itemrect.Max.y - itemrect.Min.y;
    cursor.x                -= itemwidth / 2;
    cursor.y                -= itemheight / 2;
    return cursor;
}

float CenterHorizontal()
{
    ImGuiContext& g = *ImGui::GetCurrentContext();
    ImGuiWindow*  w = g.CurrentWindow;
    ImRect        itemrect  = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    float         itemwidth = itemrect.Max.x - itemrect.Min.x;
    return (w->Size.x - itemwidth) * 0.5f;
}

void DrawGrid(float gridSize)
{
    ImVec2 winPos  = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 col = IM_COL32(88, 88, 88, 50);

    for (float y = winPos.y; y < winPos.y + winSize.y; y += gridSize)
        dl->AddLine(ImVec2(winPos.x, y), ImVec2(winPos.x + winSize.x, y), col);

    for (float x = winPos.x; x < winPos.x + winSize.x; x += gridSize)
        dl->AddLine(ImVec2(x, winPos.y), ImVec2(x, winPos.y + winSize.y), col);
}

void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

bool GrabButton(ImVec2 pos, int id)
{
    ImGui::SetCursorPos(pos);
    ImGui::PushID(id);
    ImGui::Button("*", ImVec2(15, 15));
    ImGui::PopID();
    return ImGui::IsItemActive();
}

bool IsItemActiveAlt(ImVec2 pos, int id)
{
    ImVec2 itemMin = ImGui::GetItemRectMin();
    ImVec2 itemMax = ImGui::GetItemRectMax();
    ImVec2 mouse   = ImGui::GetIO().MousePos;
    return ImGui::IsMouseDown(0) &&
           mouse.x >= itemMin.x && mouse.x <= itemMax.x &&
           mouse.y >= itemMin.y && mouse.y <= itemMax.y;
}

} // namespace shine::editor::widget::utils
