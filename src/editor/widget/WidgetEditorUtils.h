#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace shine::editor::widget::utils {

// Get cursor position in local window coordinates, centered on item
ImVec2 GetLocalCursor();

// Center a widget horizontally within the current window
float CenterHorizontal();

// Draw a grid background in the current window
void DrawGrid(float gridSize = 25.0f);

// Draw a tooltip help marker (?)
void HelpMarker(const char* desc);

// Invisible drag region for resize handles
bool GrabButton(ImVec2 pos, int id);

// Alternative IsItemActive for zero-size widgets (e.g. ProgressBar)
bool IsItemActiveAlt(ImVec2 pos, int id);

} // namespace shine::editor::widget::utils
