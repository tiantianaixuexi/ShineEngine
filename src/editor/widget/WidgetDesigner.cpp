#include "WidgetDesigner.h"
#include "WidgetEditorUtils.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_stdlib.h"
#include "fmt/format.h"
#include <string>

namespace shine::editor::widget {

namespace
{
    static bool InputTextS(const char* label, SString& value, ImGuiInputTextFlags flags = 0)
    {
        std::string temp = value.to_string();
        if (ImGui::InputText(label, &temp, flags))
        {
            value = SString::from_utf8(temp);
            return true;
        }
        return false;
    }

    static bool InputTextMultilineS(const char* label, SString& value, const ImVec2& size, ImGuiInputTextFlags flags = 0)
    {
        std::string temp = value.to_string();
        if (ImGui::InputTextMultiline(label, &temp, size, flags))
        {
            value = SString::from_utf8(temp);
            return true;
        }
        return false;
    }
}

// ──────────────────────────────────────────────────────────────
// Main Render — host window with embedded DockSpace
// ──────────────────────────────────────────────────────────────

void WidgetDesigner::Render()
{
    ImGuiID dockspaceId = ImGui::GetID("WidgetDesignerDock");

    // Create DockSpace first so the node exists
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    // Build default layout once (after DockSpace created the node)
    if (!layoutBuilt_)
    {
        BuildDefaultLayout(dockspaceId);
        layoutBuilt_ = true;
    }

    ShowPalette();
    ShowDesignerViewport();
    ShowHierarchy();
    ShowDetails();
}

// ──────────────────────────────────────────────────────────────
// Default docking layout (called once)
//   ┌──────────┬─────────────────────┬───────────┐
//   │          │                     │           │
//   │ Palette  │     Designer        │  Details  │
//   │          │                     │           │
//   │          ├─────────────────────┤           │
//   │          │     Hierarchy       │           │
//   └──────────┴─────────────────────┴───────────┘
// ──────────────────────────────────────────────────────────────

void WidgetDesigner::BuildDefaultLayout(ImGuiID dockspaceId)
{
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);

    // Use host window size; fall back to 1200x800 if too small
    ImVec2 sz = ImGui::GetWindowSize();
    if (sz.x < 400.0f) sz.x = 1200.0f;
    if (sz.y < 300.0f) sz.y = 800.0f;
    ImGui::DockBuilderSetNodeSize(dockspaceId, sz);

    ImGuiID dockLeft, dockCenter;
    ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.18f, &dockLeft, &dockCenter);

    ImGuiID dockRight;
    ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Right, 0.28f, &dockRight, &dockCenter);

    ImGuiID dockBottom;
    ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Down, 0.25f, &dockBottom, &dockCenter);

    ImGui::DockBuilderDockWindow("WD_Palette",   dockLeft);
    ImGui::DockBuilderDockWindow("WD_Designer",  dockCenter);
    ImGui::DockBuilderDockWindow("WD_Hierarchy", dockBottom);
    ImGui::DockBuilderDockWindow("WD_Details",   dockRight);

    ImGui::DockBuilderFinish(dockspaceId);
}

// ──────────────────────────────────────────────────────────────
// Palette  (Left sidebar — UE5 style category tree)
// ──────────────────────────────────────────────────────────────

void WidgetDesigner::ShowPalette()
{
    ImGui::Begin("WD_Palette", nullptr, ImGuiWindowFlags_NoCollapse);
    {
        static char searchBuf[128] = "";
        ImGui::InputTextWithHint("##search", "Search Widgets...", searchBuf, sizeof(searchBuf));
        ImGui::Separator();

        // Organize by category
        struct CategoryEntry { const char* category; EWidgetType type; };
        static const CategoryEntry entries[] = {
            {"Common",  EWidgetType::Button},
            {"Common",  EWidgetType::CheckBox},
            {"Common",  EWidgetType::Text},
            {"Common",  EWidgetType::Image},
            {"Common",  EWidgetType::ProgressBar},
            {"Common",  EWidgetType::Slider},
            {"Input",   EWidgetType::TextInput},
            {"Input",   EWidgetType::InputInt},
            {"Input",   EWidgetType::InputFloat},
            {"Input",   EWidgetType::InputFloat3},
            {"Lists",   EWidgetType::ComboBox},
            {"Lists",   EWidgetType::ListBox},
            {"Misc",    EWidgetType::Separator},
            {"Misc",    EWidgetType::Spacer},
            {"Panel",   EWidgetType::CanvasPanel},
        };

        const char* lastCategory = nullptr;
        bool categoryOpen = false;

        std::string filter(searchBuf);
        // case-insensitive filter
        for (auto& c : filter) c = (char)tolower((unsigned char)c);

        for (const auto& entry : entries)
        {
            const char* widgetName = GetWidgetTypeName(entry.type);

            // Apply search filter
            if (!filter.empty())
            {
                std::string nameLower(widgetName);
                for (auto& c : nameLower) c = (char)tolower((unsigned char)c);
                if (nameLower.find(filter) == std::string::npos)
                    continue;
            }

            // Category header
            if (lastCategory == nullptr || strcmp(lastCategory, entry.category) != 0)
            {
                if (lastCategory != nullptr && categoryOpen)
                    ImGui::TreePop();

                categoryOpen = ImGui::TreeNodeEx(entry.category, ImGuiTreeNodeFlags_DefaultOpen);
                lastCategory = entry.category;
            }

            if (!categoryOpen)
                continue;

            // Selectable widget — click to add
            if (ImGui::Selectable(widgetName))
            {
                canvas.Create(entry.type);
            }

            // Drag source for drag-and-drop into viewport
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                ImGui::SetDragDropPayload("WIDGET_TYPE", &entry.type, sizeof(EWidgetType));
                ImGui::Text("+ %s", widgetName);
                ImGui::EndDragDropSource();
            }
        }

        if (lastCategory != nullptr && categoryOpen)
            ImGui::TreePop();
    }
    ImGui::End();
}

// ──────────────────────────────────────────────────────────────
// Designer Viewport (Center — canvas with grid)
// ──────────────────────────────────────────────────────────────

void WidgetDesigner::ShowDesignerViewport()
{
    ImGui::Begin("WD_Designer", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
    {
        // Toolbar
        ImGui::Text("Canvas: %.0fx%.0f", canvas.canvasSize.x, canvas.canvasSize.y);
        ImGui::SameLine();
        ImGui::Text("| Zoom: %.0f%%", viewportTransform.zoom * 100.0f);
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset"))
        {
            viewportTransform.zoom = 1.0f;
            viewportTransform.canvasOrigin = ImVec2(0.0f, 0.0f);
        }
        ImGui::SameLine();
        ImGui::Text("| Widgets: %d", (int)canvas.widgets.size());
        if (canvas.selectedId >= 0)
        {
            WidgetItem* sel = canvas.FindWidget(canvas.selectedId);
            if (sel)
            {
                ImGui::SameLine();
                ImGui::Text("| Selected: %s", sel->name.c_str());
            }
        }
        ImGui::Separator();

        // Canvas area
        ImVec2 canvasStart = ImGui::GetCursorScreenPos();
        ImVec2 canvasArea  = ImGui::GetContentRegionAvail();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.09f, 0.09f, 1.00f));
        ImGui::BeginChild("##canvas_area", canvasArea, ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
        {
            canvas.canvasPos = ImGui::GetWindowPos();
            viewportTransform.canvasSize = canvas.canvasSize;
            viewportTransform.canvasOrigin = ImVec2(
                (canvasArea.x - canvas.canvasSize.x * viewportTransform.zoom) * 0.5f,
                (canvasArea.y - canvas.canvasSize.y * viewportTransform.zoom) * 0.5f);

            // Draw grid
            utils::DrawGrid(viewportTransform, 25.0f);

            // Right-click context menu
            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1))
                ImGui::OpenPopup("CanvasContextMenu");

            ShowContextMenu();

            // Accept drag-drop from palette
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("WIDGET_TYPE"))
                {
                    EWidgetType droppedType = *(const EWidgetType*)payload->Data;
                    canvas.Create(droppedType, true, &viewportTransform);
                }
                ImGui::EndDragDropTarget();
            }

            // Draw cursor while dragging
            if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
                ImGui::GetIO().MouseDrawCursor = true;
            else
                ImGui::GetIO().MouseDrawCursor = false;

            // Draw all widgets
            canvas.DrawAll(viewportTransform);

            // Keyboard shortcuts
            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
            {
                // Delete selected widget
                if (ImGui::IsKeyPressed(ImGuiKey_Delete) && canvas.selectedId >= 0)
                {
                    canvas.DeleteWidget(canvas.selectedId);
                    canvas.selectedId = -1;
                }
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
    ImGui::End();
}

// ──────────────────────────────────────────────────────────────
// Context Menu
// ──────────────────────────────────────────────────────────────

void WidgetDesigner::ShowContextMenu()
{
    if (ImGui::BeginPopup("CanvasContextMenu"))
    {
        if (ImGui::BeginMenu("Add Widget"))
        {
            if (ImGui::BeginMenu("Common"))
            {
                    if (ImGui::MenuItem("Button"))      canvas.Create(EWidgetType::Button, true, &viewportTransform);
                    if (ImGui::MenuItem("CheckBox"))     canvas.Create(EWidgetType::CheckBox, true, &viewportTransform);
                    if (ImGui::MenuItem("Text"))         canvas.Create(EWidgetType::Text, true, &viewportTransform);
                    if (ImGui::MenuItem("Image"))        canvas.Create(EWidgetType::Image, true, &viewportTransform);
                    if (ImGui::MenuItem("ProgressBar"))  canvas.Create(EWidgetType::ProgressBar, true, &viewportTransform);
                    if (ImGui::MenuItem("Slider"))       canvas.Create(EWidgetType::Slider, true, &viewportTransform);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Input"))
            {
                if (ImGui::MenuItem("Text Input"))   canvas.Create(EWidgetType::TextInput, true, &viewportTransform);
                if (ImGui::MenuItem("Input Int"))    canvas.Create(EWidgetType::InputInt, true, &viewportTransform);
                if (ImGui::MenuItem("Input Float"))  canvas.Create(EWidgetType::InputFloat, true, &viewportTransform);
                if (ImGui::MenuItem("Input Float3")) canvas.Create(EWidgetType::InputFloat3, true, &viewportTransform);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Lists"))
            {
                if (ImGui::MenuItem("ComboBox"))     canvas.Create(EWidgetType::ComboBox, true, &viewportTransform);
                if (ImGui::MenuItem("ListBox"))      canvas.Create(EWidgetType::ListBox, true, &viewportTransform);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Panel"))
            {
                if (ImGui::MenuItem("Canvas Panel")) canvas.Create(EWidgetType::CanvasPanel, true, &viewportTransform);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Clear All"))
        {
            canvas.Clear();
        }
        ImGui::EndPopup();
    }
}

// ──────────────────────────────────────────────────────────────
// Hierarchy  (UE5's widget tree)
// ──────────────────────────────────────────────────────────────

void WidgetDesigner::ShowHierarchy()
{
    ImGui::Begin("WD_Hierarchy", nullptr, ImGuiWindowFlags_NoCollapse);
    {
        if (canvas.widgets.empty())
        {
            ImGui::TextDisabled("(No widgets)");
        }
        else
        {
            for (auto& w : canvas.widgets)
            {
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (w.id == canvas.selectedId)
                    flags |= ImGuiTreeNodeFlags_Selected;

                bool hasChildren = !w.children.empty();
                if (hasChildren)
                    flags &= ~ImGuiTreeNodeFlags_Leaf;

                // Icon prefix by type
                const char* icon = "";
                switch (w.type)
                {
                case EWidgetType::Button:      icon = "[B] "; break;
                case EWidgetType::CheckBox:    icon = "[v] "; break;
                case EWidgetType::Text:        icon = "[T] "; break;
                case EWidgetType::Image:       icon = "[I] "; break;
                case EWidgetType::CanvasPanel: icon = "[P] "; break;
                default: icon = "    "; break;
                }

                std::string nodeLabel = fmt::format("{}{}", icon, w.name);
                bool opened = ImGui::TreeNodeEx(nodeLabel.c_str(), flags);
                if (ImGui::IsItemClicked())
                    canvas.selectedId = w.id;

                // Visibility toggle 
                ImGui::SameLine(ImGui::GetWindowWidth() - 40);
                ImGui::PushID(w.id + 100000);
                if (ImGui::SmallButton(w.visible ? "O" : "-"))
                    w.visible = !w.visible;
                ImGui::PopID();

                if (opened)
                {
                    if (hasChildren)
                    {
                        for (auto& child : w.children)
                        {
                            ImGuiTreeNodeFlags cflags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
                            if (child.id == canvas.selectedId)
                                cflags |= ImGuiTreeNodeFlags_Selected;

                            std::string childLabel = fmt::format("    {}", child.name);
                            ImGui::TreeNodeEx(childLabel.c_str(), cflags);
                            if (ImGui::IsItemClicked())
                                canvas.selectedId = child.id;
                            ImGui::TreePop();
                        }
                    }
                    ImGui::TreePop();
                }
            }
        }
    }
    ImGui::End();
}

// ──────────────────────────────────────────────────────────────
// Details Panel  (UE5's property inspector)
// ──────────────────────────────────────────────────────────────

void WidgetDesigner::ShowDetails()
{
    ImGui::Begin("WD_Details", nullptr, ImGuiWindowFlags_NoCollapse);
    {
        if (canvas.selectedId < 0 || canvas.widgets.empty())
        {
            ImGui::TextDisabled("Select a widget to edit its properties.");
            ImGui::End();
            return;
        }

        WidgetItem* w = canvas.FindWidget(canvas.selectedId);
        if (!w)
        {
            ImGui::TextDisabled("Widget not found.");
            ImGui::End();
            return;
        }

        DrawDetailsForWidget(w);
    }
    ImGui::End();
}

void WidgetDesigner::DrawDetailsForWidget(WidgetItem* w)
{
    // Header
    InputTextS("Name", w->name);
    ImGui::SameLine();
    ImGui::TextDisabled("(%s)", GetWidgetTypeName(w->type));
    ImGui::Separator();

    // ── Slot (Canvas Panel Slot) ─────────────────────────────
    if (ImGui::CollapsingHeader("Slot (Canvas Panel Slot)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Anchors preset
        const char* anchorPresets[] = {
            "Top Left", "Top Center", "Top Right",
            "Center Left", "Center", "Center Right",
            "Bottom Left", "Bottom Center", "Bottom Right",
            "Top Stretch", "Bottom Stretch", "Left Stretch", "Right Stretch",
            "Full Stretch", "Custom"
        };
        int anchorIdx = (int)w->anchor;
        if (ImGui::Combo("Anchors", &anchorIdx, anchorPresets, IM_ARRAYSIZE(anchorPresets)))
            w->anchor = (EAnchorPreset)anchorIdx;

        ImGui::DragFloat2("Position", &w->position.x, 1.0f);
        ImGui::DragFloat2("Size", &w->size.x, 1.0f);
        ImGui::DragFloat2("Alignment", &w->alignment.x, 0.01f, 0.0f, 1.0f);
        ImGui::Checkbox("Auto Size", &w->autoSize);
        ImGui::DragInt("ZOrder", &w->zOrder, 1);
    }

    // ── Appearance ────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Appearance", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit4("Tint", &w->tintColor.x);
        ImGui::SliderFloat("Opacity", &w->opacity, 0.0f, 1.0f);
    }

    // ── Content (widget-type specific) ────────────────────────
    if (ImGui::CollapsingHeader("Content", ImGuiTreeNodeFlags_DefaultOpen))
    {
        switch (w->type)
        {
        case EWidgetType::Button:
            InputTextS("Label", w->label);
            break;
        case EWidgetType::CheckBox:
            InputTextS("Label", w->label);
            ImGui::Checkbox("Value", &w->boolValue);
            break;
        case EWidgetType::Text:
            InputTextMultilineS("Text", w->textValue, ImVec2(-1, 60));
            break;
        case EWidgetType::Image:
            ImGui::TextDisabled("(Image source placeholder)");
            break;
        case EWidgetType::ProgressBar:
            ImGui::SliderFloat("Progress", &w->progress, 0.0f, 1.0f);
            break;
        case EWidgetType::Slider:
            InputTextS("Label", w->label);
            ImGui::DragFloat("Value", &w->floatValue, 0.01f);
            ImGui::DragFloat("Min", &w->minValue, 0.01f);
            ImGui::DragFloat("Max", &w->maxValue, 0.01f);
            break;
        case EWidgetType::TextInput:
            InputTextS("Label", w->label);
            InputTextS("Value", w->textValue);
            ImGui::DragFloat("Width", &w->itemWidth, 1.0f, 50.0f, 500.0f);
            break;
        case EWidgetType::InputInt:
            InputTextS("Label", w->label);
            ImGui::InputInt("Value", &w->intValue);
            ImGui::DragFloat("Width", &w->itemWidth, 1.0f, 50.0f, 500.0f);
            break;
        case EWidgetType::InputFloat:
            InputTextS("Label", w->label);
            ImGui::InputFloat("Value", &w->floatValue);
            ImGui::DragFloat("Width", &w->itemWidth, 1.0f, 50.0f, 500.0f);
            break;
        case EWidgetType::InputFloat3:
            InputTextS("Label", w->label);
            ImGui::InputFloat3("Value", w->floatVec);
            ImGui::DragFloat("Width", &w->itemWidth, 1.0f, 50.0f, 500.0f);
            break;
        case EWidgetType::ComboBox:
            InputTextS("Label", w->label);
            ImGui::DragFloat("Width", &w->itemWidth, 1.0f, 50.0f, 500.0f);
            break;
        case EWidgetType::ListBox:
            InputTextS("Label", w->label);
            ImGui::DragFloat("Width", &w->itemWidth, 1.0f, 50.0f, 500.0f);
            break;
        case EWidgetType::CanvasPanel:
            ImGui::DragFloat2("Panel Size", &w->size.x, 1.0f);
            break;
        default:
            ImGui::TextDisabled("(No editable content)");
            break;
        }
    }

    // ── Behavior ──────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Behavior"))
    {
        ImGui::Checkbox("Is Enabled", &w->isEnabled);
        ImGui::Checkbox("Visible", &w->visible);
        ImGui::Checkbox("Locked", &w->locked);
    }

    ImGui::Separator();
    ImGui::Spacing();

    // Delete button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
    if (ImGui::Button("Delete Widget", ImVec2(-1, 0)))
    {
        canvas.DeleteWidget(w->id);
        canvas.selectedId = -1;
    }
    ImGui::PopStyleColor();
}

} // namespace shine::editor::widget
