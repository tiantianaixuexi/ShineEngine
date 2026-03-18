#pragma once

#include "WidgetObject.h"
#include "WidgetEditorUtils.h"
#include "imgui/imgui.h"

namespace shine::editor::widget {

// UE5-style Widget Editor with:
//  - Palette panel (left sidebar with widget types organized by category)
//  - Designer viewport (canvas with grid, drag-and-drop widgets)
//  - Hierarchy panel (widget tree view)
//  - Details panel (properties inspector for selected widget)
// All panels are docked inside a single host window.
class WidgetDesigner
{
public:
    WidgetDesigner() = default;

    // Call this every frame from your editor view
    void Render();

    // Access
    WidgetCanvas& GetCanvas() { return canvas; }

private:
    void BuildDefaultLayout(ImGuiID dockspaceId);
    void ShowPalette();
    void ShowDesignerViewport();
    void ShowHierarchy();
    void ShowDetails();
    void ShowContextMenu();

    // Properties panel per-type helpers
    void DrawDetailsForWidget(WidgetItem* w);

private:
    WidgetCanvas canvas;
    utils::WidgetViewportTransform viewportTransform;
    bool         layoutBuilt_      = false;
};

} // namespace shine::editor::widget
