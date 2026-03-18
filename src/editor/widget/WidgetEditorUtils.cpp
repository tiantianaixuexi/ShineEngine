#include "WidgetEditorUtils.h"

namespace shine::editor::widget::utils {

ImVec2 GetLocalCursor(const WidgetViewportTransform& transform)
{
    ImGuiIO&      io         = ImGui::GetIO();
    ImGuiContext& g          = *ImGui::GetCurrentContext();
    ImGuiWindow*  w          = g.CurrentWindow;
    ImVec2        cursor     = ImVec2(io.MousePos.x - w->Pos.x - transform.canvasOrigin.x,
                                      io.MousePos.y - w->Pos.y - transform.canvasOrigin.y);
    ImRect        itemrect   = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    float         itemwidth  = (itemrect.Max.x - itemrect.Min.x) / ImMax(transform.zoom, 0.0001f);
    float         itemheight = (itemrect.Max.y - itemrect.Min.y) / ImMax(transform.zoom, 0.0001f);
    cursor.x                = cursor.x / ImMax(transform.zoom, 0.0001f) - itemwidth / 2.0f;
    cursor.y                = cursor.y / ImMax(transform.zoom, 0.0001f) - itemheight / 2.0f;
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

void DrawGrid(const WidgetViewportTransform& transform, float gridSize)
{
    ImVec2 winPos  = ImGui::GetWindowPos();
    ImVec2 winSize = ImVec2(transform.canvasSize.x * transform.zoom,
                            transform.canvasSize.y * transform.zoom);
    ImVec2 gridOrigin = ImVec2(winPos.x + transform.canvasOrigin.x, winPos.y + transform.canvasOrigin.y);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 col = IM_COL32(88, 88, 88, 50);
    const float step = ImMax(gridSize * transform.zoom, 4.0f);

    for (float y = gridOrigin.y; y < gridOrigin.y + winSize.y; y += step)
        dl->AddLine(ImVec2(gridOrigin.x, y), ImVec2(gridOrigin.x + winSize.x, y), col);

    for (float x = gridOrigin.x; x < gridOrigin.x + winSize.x; x += step)
        dl->AddLine(ImVec2(x, gridOrigin.y), ImVec2(x, gridOrigin.y + winSize.y), col);
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
