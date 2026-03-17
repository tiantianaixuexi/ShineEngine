#pragma once

#include <string>
#include <vector>
#include "imgui/imgui.h"

namespace shine::editor::widget {

// Widget type enum, similar to UE5's widget palette categories
enum class EWidgetType : int
{
    // Common
    Button,
    CheckBox,
    Text,
    Image,
    ProgressBar,
    Slider,
    // Input
    TextInput,
    InputInt,
    InputFloat,
    InputFloat3,
    // Lists
    ComboBox,
    ListBox,
    // Misc
    Separator,
    Spacer,
    // Panel
    CanvasPanel,
    // Count
    COUNT
};

const char* GetWidgetTypeName(EWidgetType type);
const char* GetWidgetTypeCategory(EWidgetType type);

// Anchor preset enum, similar to UE5 Canvas Panel Slot anchors
enum class EAnchorPreset : int
{
    TopLeft,
    TopCenter,
    TopRight,
    CenterLeft,
    Center,
    CenterRight,
    BottomLeft,
    BottomCenter,
    BottomRight,
    TopStretch,
    BottomStretch,
    LeftStretch,
    RightStretch,
    FullStretch,
    Custom
};

// A single widget in the editor (like UWidget in UE5)
struct WidgetItem
{
    int              id          = 0;
    EWidgetType      type        = EWidgetType::Button;
    std::string      name        = "Widget";
    bool             alive       = true;
    bool             visible     = true;
    bool             isEnabled   = true;
    bool             locked      = false;

    // Slot (Canvas Panel Slot in UE5)
    ImVec2           position    = ImVec2(100, 100);
    ImVec2           size        = ImVec2(100, 30);
    bool             autoSize    = true;
    ImVec2           alignment   = ImVec2(0, 0);
    EAnchorPreset    anchor      = EAnchorPreset::TopLeft;
    int              zOrder      = 0;

    // Appearance
    ImVec4           tintColor   = ImVec4(1, 1, 1, 1);
    float            opacity     = 1.0f;

    // Widget-specific values
    std::string      label       = "Label";
    std::string      textValue   = "";
    bool             boolValue   = false;
    float            floatValue  = 0.0f;
    int              intValue    = 0;
    float            floatVec[4] = {0, 0, 0, 0};
    float            progress    = 0.5f;
    float            minValue    = 0.0f;
    float            maxValue    = 1.0f;
    float            itemWidth   = 200.0f;

    // Children (for panel widgets like CanvasPanel)
    std::vector<WidgetItem> children;

    // Internal state
    bool             initialized = false;

    void Draw(int* selectedId, bool isDesignMode);
    void Delete() { alive = false; }

private:
    void DrawHighlight(int* selectedId);
    void HandleDrag(int* selectedId);
};

// Canvas that holds all top-level widgets
struct WidgetCanvas
{
    bool                    active       = true;
    ImVec2                  canvasSize   = ImVec2(1280, 720);
    ImVec2                  canvasPos    = {};
    int                     idGen        = 0;
    int                     selectedId   = -1;
    std::vector<WidgetItem> widgets;

    void DrawAll();
    void Create(EWidgetType type, bool atCursor = false);
    WidgetItem* FindWidget(int id);
    void DeleteWidget(int id);
    void Clear();
};

} // namespace shine::editor::widget
