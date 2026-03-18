#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace shine::editor::widget::utils {

struct WidgetViewportTransform
{
	ImVec2 canvasOrigin = ImVec2(0.0f, 0.0f);
	ImVec2 canvasSize   = ImVec2(1280.0f, 720.0f);
	float  zoom         = 1.0f;
};

// Get cursor position in local window coordinates, centered on item
ImVec2 GetLocalCursor(const WidgetViewportTransform& transform);

// Center a widget horizontally within the current window
float CenterHorizontal();

// Draw a grid background in the current window
void DrawGrid(const WidgetViewportTransform& transform, float gridSize = 25.0f);

// Draw a tooltip help marker (?)
void HelpMarker(const char* desc);

// Invisible drag region for resize handles
bool GrabButton(ImVec2 pos, int id);

// Alternative IsItemActive for zero-size widgets (e.g. ProgressBar)
bool IsItemActiveAlt(ImVec2 pos, int id);

} // namespace shine::editor::widget::utils
