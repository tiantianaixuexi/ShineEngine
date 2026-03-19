#include "WidgetObject.h"

#include <array>
#include <string>



#include "imgui/imgui_internal.h"
#include "imgui/imgui_stdlib.h"
#include "fmt/format.h"
#include "glaze/json.hpp"

#include "WidgetEditorUtils.h"

namespace shine::editor::widget {

namespace detail
{
    struct WidgetItemJson
    {
        int id = 0;
        int type = 0;
        std::string name;
        bool alive = true;
        bool visible = true;
        bool isEnabled = true;
        bool locked = false;

        std::array<float, 2> position = {100.0f, 100.0f};
        std::array<float, 2> size = {100.0f, 30.0f};
        bool autoSize = true;
        std::array<float, 2> alignment = {0.0f, 0.0f};
        int anchor = 0;
        int zOrder = 0;

        std::array<float, 4> tintColor = {1.0f, 1.0f, 1.0f, 1.0f};
        float opacity = 1.0f;

        std::string label;
        std::string textValue;
        bool boolValue = false;
        float floatValue = 0.0f;
        int intValue = 0;
        std::array<float, 4> floatVec = {0.0f, 0.0f, 0.0f, 0.0f};
        float progress = 0.5f;
        float minValue = 0.0f;
        float maxValue = 1.0f;
        float itemWidth = 200.0f;

        std::vector<WidgetItemJson> children {};
    };

    struct WidgetCanvasJson
    {
        std::array<float, 2> canvasSize = {1280.0f, 720.0f};
        bool portraitMode = false;
        int idGen = 0;
        int selectedId = -1;
        std::vector<WidgetItemJson> widgets;
    };
}

namespace
{
    static bool InputTextS(const char* label, SString& value, ImGuiInputTextFlags flags = 0)
    {

        if (ImGui::InputText(label, &value, flags))
        {
            return true;
        }
        return false;
    }

    static bool InputTextMultilineS(const char* label, SString& value, const ImVec2& size, ImGuiInputTextFlags flags = 0)
    {

        if (ImGui::InputTextMultiline(label, &value, size, flags))
        {
            return true;
        }
        return false;
    }

    static detail::WidgetItemJson ToJson(const WidgetItem& item)
    {
        detail::WidgetItemJson json;
        json.id = item.id;
        json.type = static_cast<int>(item.type);
        json.name = item.name.to_string();
        json.alive = item.alive;
        json.visible = item.visible;
        json.isEnabled = item.isEnabled;
        json.locked = item.locked;
        json.position = {item.position.x, item.position.y};
        json.size = {item.size.x, item.size.y};
        json.autoSize = item.autoSize;
        json.alignment = {item.alignment.x, item.alignment.y};
        json.anchor = static_cast<int>(item.anchor);
        json.zOrder = item.zOrder;
        json.tintColor = {item.tintColor.x, item.tintColor.y, item.tintColor.z, item.tintColor.w};
        json.opacity = item.opacity;
        json.label = item.label.to_string();
        json.textValue = item.textValue.to_string();
        json.boolValue = item.boolValue;
        json.floatValue = item.floatValue;
        json.intValue = item.intValue;
        json.floatVec = {item.floatVec[0], item.floatVec[1], item.floatVec[2], item.floatVec[3]};
        json.progress = item.progress;
        json.minValue = item.minValue;
        json.maxValue = item.maxValue;
        json.itemWidth = item.itemWidth;
        json.children.reserve(item.children.size());
        for (const auto& child : item.children)
            json.children.push_back(ToJson(child));
        return json;
    }

    static void FromJson(const detail::WidgetItemJson& json, WidgetItem& item)
    {
        item.id = json.id;
        item.type = static_cast<EWidgetType>(json.type);
        item.name = SString::from_utf8(json.name);
        item.alive = json.alive;
        item.visible = json.visible;
        item.isEnabled = json.isEnabled;
        item.locked = json.locked;
        item.position = ImVec2(json.position[0], json.position[1]);
        item.size = ImVec2(json.size[0], json.size[1]);
        item.autoSize = json.autoSize;
        item.alignment = ImVec2(json.alignment[0], json.alignment[1]);
        item.anchor = static_cast<EAnchorPreset>(json.anchor);
        item.zOrder = json.zOrder;
        item.tintColor = ImVec4(json.tintColor[0], json.tintColor[1], json.tintColor[2], json.tintColor[3]);
        item.opacity = json.opacity;
        item.label = SString::from_utf8(json.label);
        item.textValue = SString::from_utf8(json.textValue);
        item.boolValue = json.boolValue;
        item.floatValue = json.floatValue;
        item.intValue = json.intValue;
        item.floatVec[0] = json.floatVec[0];
        item.floatVec[1] = json.floatVec[1];
        item.floatVec[2] = json.floatVec[2];
        item.floatVec[3] = json.floatVec[3];
        item.progress = json.progress;
        item.minValue = json.minValue;
        item.maxValue = json.maxValue;
        item.itemWidth = json.itemWidth;
        item.children.clear();
        item.children.reserve(json.children.size());
        for (const auto& childJson : json.children)
        {
            item.children.emplace_back();
            FromJson(childJson, item.children.back());
        }
    }
}

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

void WidgetItem::HandleDrag(int* selectedId, const utils::WidgetViewportTransform& transform)
{
    if (!locked && ImGui::IsItemActive())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        position = utils::GetLocalCursor(transform);
        *selectedId = id;
    }
}

// ──────────────────────────────────────────────────────────────
// WidgetItem::Draw
// ──────────────────────────────────────────────────────────────

void WidgetItem::Draw(int* selectedId, bool isDesignMode, const utils::WidgetViewportTransform& transform)
{
    if (!alive || !visible)
        return;

    ImGui::PushID(id);

    if (isDesignMode)
        ImGui::SetCursorPos(ImVec2(transform.canvasOrigin.x + position.x * transform.zoom,
                                   transform.canvasOrigin.y + position.y * transform.zoom));

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

    const float zoom = ImMax(transform.zoom, 0.0001f);
    ImVec2 drawSize = autoSize ? ImVec2(0, 0) : ImVec2(size.x * zoom, size.y * zoom);

    switch (type)
    {
    case EWidgetType::Button:
    {
        ImGui::Button(label.c_str(), drawSize);
        if (autoSize)
            size = ImVec2(ImGui::GetItemRectSize().x / zoom, ImGui::GetItemRectSize().y / zoom);
        HandleDrag(selectedId, transform);
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::CheckBox:
    {
        ImGui::Checkbox(label.c_str(), &boolValue);
        HandleDrag(selectedId, transform);
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
            ImGui::SetCursorPos(ImVec2(transform.canvasOrigin.x + position.x * zoom,
                                       transform.canvasOrigin.y + position.y * zoom));
            ImGui::InvisibleButton("##drag", ImVec2(ts.x * zoom, ts.y * zoom));
        }
        HandleDrag(selectedId, transform);
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::Image:
    {
        ImVec2 imgSize = autoSize ? ImVec2(100 * zoom, 100 * zoom) : ImVec2(size.x * zoom, size.y * zoom);
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
        HandleDrag(selectedId, transform);
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::ProgressBar:
    {
        ImVec2 barSize = autoSize ? ImVec2(0, 0) : ImVec2(size.x * zoom, size.y * zoom);
        ImGui::ProgressBar(progress, barSize);
        if (!locked && utils::IsItemActiveAlt(position, id))
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            position = utils::GetLocalCursor(transform);
            *selectedId = id;
        }
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::Slider:
    {
        ImGui::PushItemWidth(autoSize ? itemWidth * zoom : size.x * zoom);
        ImGui::SliderFloat(label.c_str(), &floatValue, minValue, maxValue);
        ImGui::PopItemWidth();
        HandleDrag(selectedId, transform);
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::TextInput:
    {
        ImGui::PushItemWidth(autoSize ? itemWidth * zoom : size.x * zoom);
        InputTextS(label.c_str(), textValue);
        ImGui::PopItemWidth();
        if (!locked && ImGui::IsItemActive() && ImGui::IsMouseDown(0))
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            position = utils::GetLocalCursor(transform);
            *selectedId = id;
        }
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::InputInt:
    {
        ImGui::PushItemWidth(autoSize ? itemWidth * zoom : size.x * zoom);
        ImGui::InputInt(label.c_str(), &intValue);
        ImGui::PopItemWidth();
        if (!locked && ImGui::IsItemActive() && ImGui::IsMouseDown(0))
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            position = utils::GetLocalCursor(transform);
            *selectedId = id;
        }
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::InputFloat:
    {
        ImGui::PushItemWidth(autoSize ? itemWidth * zoom : size.x * zoom);
        ImGui::InputFloat(label.c_str(), &floatValue, 0.01f, 1.0f, "%.3f");
        ImGui::PopItemWidth();
        if (!locked && ImGui::IsItemActive() && ImGui::IsMouseDown(0))
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            position = utils::GetLocalCursor(transform);
            *selectedId = id;
        }
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::InputFloat3:
    {
        ImGui::PushItemWidth(autoSize ? itemWidth * zoom : size.x * zoom);
        ImGui::InputFloat3(label.c_str(), floatVec);
        ImGui::PopItemWidth();
        if (!locked && ImGui::IsItemActive() && ImGui::IsMouseDown(0))
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            position = utils::GetLocalCursor(transform);
            *selectedId = id;
        }
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::ComboBox:
    {
        ImGui::PushItemWidth(autoSize ? itemWidth * zoom : size.x * zoom);
        const char* items[] = {"Item 0", "Item 1", "Item 2", "Item 3"};
        ImGui::Combo(label.c_str(), &intValue, items, IM_ARRAYSIZE(items));
        ImGui::PopItemWidth();
        HandleDrag(selectedId, transform);
        DrawHighlight(selectedId);
        break;
    }
    case EWidgetType::ListBox:
    {
        ImGui::PushItemWidth(autoSize ? itemWidth * zoom : size.x * zoom);
        const char* items[] = {"Item 0", "Item 1", "Item 2", "Item 3"};
        ImGui::ListBox(label.c_str(), &intValue, items, IM_ARRAYSIZE(items));
        ImGui::PopItemWidth();
        HandleDrag(selectedId, transform);
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
        ImVec2 spacerSize = autoSize ? ImVec2(100 * zoom, 20 * zoom) : ImVec2(size.x * zoom, size.y * zoom);
        ImGui::Dummy(spacerSize);
        break;
    }
    case EWidgetType::CanvasPanel:
    {
        ImVec2 panelSize = autoSize ? ImVec2(300 * zoom, 200 * zoom) : ImVec2(size.x * zoom, size.y * zoom);
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
                it->Draw(selectedId, isDesignMode, transform);
                ++it;
            }
        }
        ImGui::EndChild();
        HandleDrag(selectedId, transform);
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

SString WidgetItem::SerializeJson(bool prettify) const
{
    auto data = ToJson(*this);
    auto result = glz::write_json(data);
    if (!result)
        return SString{};

    std::string json = std::move(result.value());
    if (prettify)
        json = glz::prettify_json(json);
    return SString::from_utf8(json);
}

bool WidgetItem::DeserializeJson(STextView json)
{
    detail::WidgetItemJson data;
    if (auto ec = glz::read_json(data, json); ec)
        return false;

    FromJson(data, *this);
    return true;
}

// ──────────────────────────────────────────────────────────────
// WidgetCanvas
// ──────────────────────────────────────────────────────────────

ImVec2 WidgetCanvas::GetResolvedCanvasSize() const
{
    return portraitMode ? ImVec2(canvasSize.y, canvasSize.x) : canvasSize;
}

void WidgetCanvas::DrawAll(const utils::WidgetViewportTransform& transform)
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
            it->Draw(&selectedId, true, transform);
            ++it;
        }
    }
}

void WidgetCanvas::Create(EWidgetType type, bool atCursor, const utils::WidgetViewportTransform* transform)
{
    idGen++;
    WidgetItem w;
    w.id   = idGen;
    w.type = type;
    w.name = SString::from_utf8(fmt::format("{}_{}", GetWidgetTypeName(type), idGen));
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
        const float zoom = transform ? ImMax(transform->zoom, 0.0001f) : 1.0f;
        const ImVec2 origin = transform ? transform->canvasOrigin : ImVec2(0.0f, 0.0f);
        w.position = ImVec2((ImGui::GetMousePos().x - canvasPos.x - origin.x) / zoom,
                            (ImGui::GetMousePos().y - canvasPos.y - origin.y) / zoom);
    }

    widgets.push_back(w);
    selectedId = idGen;
}

SString WidgetCanvas::SaveToJson(bool prettify) const
{
    detail::WidgetCanvasJson data;
    data.canvasSize = {canvasSize.x, canvasSize.y};
    data.portraitMode = portraitMode;
    data.idGen = idGen;
    data.selectedId = selectedId;
    data.widgets.reserve(widgets.size());
    for (const auto& widget : widgets)
        data.widgets.push_back(ToJson(widget));

    auto result = glz::write_json(data);
    if (!result)
        return SString{};

    std::string json = std::move(result.value());
    if (prettify)
        json = glz::prettify_json(json);
    return SString::from_utf8(json);
}

bool WidgetCanvas::LoadFromJson(STextView json)
{
    detail::WidgetCanvasJson data;
    if (auto ec = glz::read_json(data, json); ec)
        return false;

    canvasSize = ImVec2(data.canvasSize[0], data.canvasSize[1]);
    portraitMode = data.portraitMode;
    idGen = data.idGen;
    selectedId = data.selectedId;
    widgets.clear();
    widgets.reserve(data.widgets.size());
    for (const auto& widgetJson : data.widgets)
    {
        widgets.emplace_back();
        FromJson(widgetJson, widgets.back());
    }
    return true;
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
