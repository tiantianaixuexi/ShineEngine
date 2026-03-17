#include "WidgetObject.h"
#include "WidgetEditorUtils.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_stdlib.h"
#include "fmt/format.h"

namespace shine::editor::widget {

// ──────────────────────────────────────────────────────────────
// Widget Type Names / Categories
// ──────────────────────────────────────────────────────────────

const char* GetWidgetTypeName(EWidgetType type)
{
    switch (type)
    {
    case EWidgetType::Button:       return "Button";
    case EWidgetType::CheckBox:     return "CheckBox";
    case EWidgetType::Text:         return "Text";
    case EWidgetType::Image:        return "Image";
    case EWidgetType::ProgressBar:  return "ProgressBar";
    case EWidgetType::Slider:       return "Slider";
    case EWidgetType::TextInput:    return "TextInput";
    case EWidgetType::InputInt:     return "InputInt";
    case EWidgetType::InputFloat:   return "InputFloat";
    case EWidgetType::InputFloat3:  return "InputFloat3";
    case EWidgetType::ComboBox:     return "ComboBox";
    case EWidgetType::ListBox:      return "ListBox";
    case EWidgetType::Separator:    return "Separator";
    case EWidgetType::Spacer:       return "Spacer";
    case EWidgetType::CanvasPanel:  return "CanvasPanel";
    default: return "Unknown";
    }
}

const char* GetWidgetTypeCategory(EWidgetType type)
{
    switch (type)
    {
    case EWidgetType::Button:
    case EWidgetType::CheckBox:
    case EWidgetType::Text:
    case EWidgetType::Image:
    case EWidgetType::ProgressBar:
    case EWidgetType::Slider:
        return "Common";
    case EWidgetType::TextInput:
    case EWidgetType::InputInt:
    case EWidgetType::InputFloat:
    case EWidgetType::InputFloat3:
        return "Input";
    case EWidgetType::ComboBox:
    case EWidgetType::ListBox:
        return "Lists";
    case EWidgetType::Separator:
    case EWidgetType::Spacer:
        return "Misc";
    case EWidgetType::CanvasPanel:
        return "Panel";
    default: return "Misc";
    }
}

// ──────────────────────────────────────────────────────────────
// WidgetItem::DrawHighlight
// ──────────────────────────────────────────────────────────────

void WidgetItem::DrawHighlight(int* selectedId)
{
    if (id == *selectedId)
    {
        ImRect r(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        r.Expand(3.0f);
        ImGui::GetForegroundDrawList()->AddRect(r.Min, r.Max, IM_COL32(60, 200, 60, 255), 0.0f, 0, 2.0f);
    }
}

void WidgetItem::HandleDrag(int* selectedId)
{
    if (!locked && ImGui::IsItemActive())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        position = utils::GetLocalCursor();
        *selectedId = id;
    }
}

// ──────────────────────────────────────────────────────────────
// WidgetItem::Draw
// ──────────────────────────────────────────────────────────────

void WidgetItem::Draw(int* selectedId, bool isDesignMode)
{
    if (!alive || !visible)
        return;

    ImGui::PushID(id);

    if (isDesignMode)
        ImGui::SetCursorPos(position);

    // Apply tint color & opacity to relevant style colors
    const float alpha = tintColor.w * opacity;
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

    const bool hasTint = (tintColor.x != 1.0f || tintColor.y != 1.0f || tintColor.z != 1.0f);
    if (hasTint)
    {
        auto tintCol = [&](ImVec4 base) -> ImVec4 {
            return ImVec4(base.x * tintColor.x, base.y * tintColor.y, base.z * tintColor.z, base.w);
        };
        ImGuiStyle& s = ImGui::GetStyle();
        ImGui::PushStyleColor(ImGuiCol_Button,        tintCol(s.Colors[ImGuiCol_Button]));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  tintCol(s.Colors[ImGuiCol_ButtonHovered]));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   tintCol(s.Colors[ImGuiCol_ButtonActive]));
        ImGui::PushStyleColor(ImGuiCol_FrameBg,        tintCol(s.Colors[ImGuiCol_FrameBg]));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,  tintCol(s.Colors[ImGuiCol_FrameBgHovered]));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,   tintCol(s.Colors[ImGuiCol_FrameBgActive]));
        ImGui::PushStyleColor(ImGuiCol_CheckMark,      tintCol(s.Colors[ImGuiCol_CheckMark]));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab,     tintCol(s.Colors[ImGuiCol_SliderGrab]));
        ImGui::PushStyleColor(ImGuiCol_Text,           tintCol(s.Colors[ImGuiCol_Text]));
    }

    ImVec2 drawSize = autoSize ? ImVec2(0, 0) : size;

    switch (type)
    {
    case EWidgetType::Button:
    {
        ImGui::Button(label.c_str(), drawSize);
        if (autoSize)
            size = ImGui::GetItemRectSize();
        HandleDrag(selectedId);
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::CheckBox:
    {
        ImGui::Checkbox(label.c_str(), &boolValue);
        HandleDrag(selectedId);
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::Text:
    {
        ImGui::TextUnformatted(textValue.c_str());
        if (isDesignMode)
        {
            ImVec2 ts = ImGui::CalcTextSize(textValue.c_str());
            if (ts.x < 1) ts.x = 1;
            ImGui::SetCursorPos(position);
            ImGui::InvisibleButton("##drag", ts);
        }
        HandleDrag(selectedId);
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::Image:
    {
        ImVec2 imgSize = autoSize ? ImVec2(100, 100) : size;
        ImVec2 cursorScreen = ImGui::GetCursorScreenPos();
        ImVec4 fillCol = ImVec4(tintColor.x, tintColor.y, tintColor.z, alpha);
        ImGui::GetWindowDrawList()->AddRectFilled(
            cursorScreen,
            ImVec2(cursorScreen.x + imgSize.x, cursorScreen.y + imgSize.y),
            ImGui::GetColorU32(fillCol));
        ImGui::GetWindowDrawList()->AddRect(
            cursorScreen,
            ImVec2(cursorScreen.x + imgSize.x, cursorScreen.y + imgSize.y),
            IM_COL32(200, 200, 200, 255));
        ImGui::Dummy(imgSize);
        HandleDrag(selectedId);
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::ProgressBar:
    {
        ImVec2 barSize = autoSize ? ImVec2(0, 0) : ImVec2(size.x, size.y);
        ImGui::ProgressBar(progress, barSize);
        if (!locked && utils::IsItemActiveAlt(position, id))
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            position = utils::GetLocalCursor();
            *selectedId = id;
        }
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::Slider:
    {
        ImGui::PushItemWidth(autoSize ? itemWidth : size.x);
        ImGui::SliderFloat(label.c_str(), &floatValue, minValue, maxValue);
        ImGui::PopItemWidth();
        HandleDrag(selectedId);
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::TextInput:
    {
        ImGui::PushItemWidth(autoSize ? itemWidth : size.x);
        ImGui::InputText(label.c_str(), &textValue);
        ImGui::PopItemWidth();
        if (!locked && ImGui::IsItemActive() && ImGui::IsMouseDown(0))
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            position = utils::GetLocalCursor();
            *selectedId = id;
        }
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::InputInt:
    {
        ImGui::PushItemWidth(autoSize ? itemWidth : size.x);
        ImGui::InputInt(label.c_str(), &intValue);
        ImGui::PopItemWidth();
        if (!locked && ImGui::IsItemActive() && ImGui::IsMouseDown(0))
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            position = utils::GetLocalCursor();
            *selectedId = id;
        }
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::InputFloat:
    {
        ImGui::PushItemWidth(autoSize ? itemWidth : size.x);
        ImGui::InputFloat(label.c_str(), &floatValue, 0.01f, 1.0f, "%.3f");
        ImGui::PopItemWidth();
        if (!locked && ImGui::IsItemActive() && ImGui::IsMouseDown(0))
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            position = utils::GetLocalCursor();
            *selectedId = id;
        }
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::InputFloat3:
    {
        ImGui::PushItemWidth(autoSize ? itemWidth : size.x);
        ImGui::InputFloat3(label.c_str(), floatVec);
        ImGui::PopItemWidth();
        if (!locked && ImGui::IsItemActive() && ImGui::IsMouseDown(0))
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            position = utils::GetLocalCursor();
            *selectedId = id;
        }
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::ComboBox:
    {
        ImGui::PushItemWidth(autoSize ? itemWidth : size.x);
        const char* items[] = {"Item 0", "Item 1", "Item 2", "Item 3"};
        ImGui::Combo(label.c_str(), &intValue, items, IM_ARRAYSIZE(items));
        ImGui::PopItemWidth();
        HandleDrag(selectedId);
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::ListBox:
    {
        ImGui::PushItemWidth(autoSize ? itemWidth : size.x);
        const char* items[] = {"Item 0", "Item 1", "Item 2", "Item 3"};
        ImGui::ListBox(label.c_str(), &intValue, items, IM_ARRAYSIZE(items));
        ImGui::PopItemWidth();
        HandleDrag(selectedId);
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::Separator:
    {
        ImGui::Separator();
        break;
    }
    case EWidgetType::Spacer:
    {
        ImVec2 spacerSize = autoSize ? ImVec2(100, 20) : size;
        ImGui::Dummy(spacerSize);
        break;
    }
    case EWidgetType::CanvasPanel:
    {
        ImVec2 panelSize = autoSize ? ImVec2(300, 200) : size;
        ImGui::BeginChild(id + 10000, panelSize, ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);
        for (auto it = children.begin(); it != children.end();)
        {
            if (!it->alive)
            {
                it = children.erase(it);
            }
            else
            {
                it->Draw(selectedId, isDesignMode);
                ++it;
            }
        }
        ImGui::EndChild();
        HandleDrag(selectedId);
        DrawHighlight(selectedId);
        break;
    }
    default:
        break;
    }

    if (hasTint)
        ImGui::PopStyleColor(9);
    ImGui::PopStyleVar(); // Alpha

    ImGui::PopID();
}

// ──────────────────────────────────────────────────────────────
// WidgetCanvas
// ──────────────────────────────────────────────────────────────

void WidgetCanvas::DrawAll()
{
    if (!active)
        return;

    for (auto it = widgets.begin(); it != widgets.end();)
    {
        if (!it->alive)
        {
            it = widgets.erase(it);
        }
        else
        {
            it->Draw(&selectedId, true);
            ++it;
        }
    }
}

void WidgetCanvas::Create(EWidgetType type, bool atCursor)
{
    idGen++;
    WidgetItem w;
    w.id   = idGen;
    w.type = type;
    w.name = fmt::format("{}_{}", GetWidgetTypeName(type), idGen);
    w.label = w.name;

    // Set sensible defaults per type
    switch (type)
    {
    case EWidgetType::Text:
        w.textValue = "Text Block";
        w.autoSize  = true;
        break;
    case EWidgetType::Button:
        w.label     = "Button";
        w.autoSize  = true;
        break;
    case EWidgetType::ProgressBar:
        w.progress  = 0.5f;
        break;
    case EWidgetType::Slider:
        w.minValue  = 0.0f;
        w.maxValue  = 1.0f;
        break;
    case EWidgetType::Image:
        w.size      = ImVec2(100, 100);
        w.autoSize  = false;
        break;
    case EWidgetType::CanvasPanel:
        w.size      = ImVec2(300, 200);
        w.autoSize  = false;
        break;
    default:
        break;
    }

    if (atCursor)
    {
        w.position = ImVec2(ImGui::GetMousePos().x - canvasPos.x,
                            ImGui::GetMousePos().y - canvasPos.y);
    }

    widgets.push_back(w);
    selectedId = idGen;
}

WidgetItem* WidgetCanvas::FindWidget(int id)
{
    for (auto& w : widgets)
    {
        if (w.id == id)
            return &w;
        // Search in children of panel widgets
        for (auto& c : w.children)
        {
            if (c.id == id)
                return &c;
        }
    }
    return nullptr;
}

void WidgetCanvas::DeleteWidget(int id)
{
    for (auto it = widgets.begin(); it != widgets.end(); ++it)
    {
        if (it->id == id)
        {
            widgets.erase(it);
            return;
        }
        for (auto ct = it->children.begin(); ct != it->children.end(); ++ct)
        {
            if (ct->id == id)
            {
                it->children.erase(ct);
                return;
            }
        }
    }
}

void WidgetCanvas::Clear()
{
    widgets.clear();
    selectedId = -1;
    idGen = 0;
}

} // namespace shine::editor::widget
