#pragma once

#include <array>
#include <string>
#include <vector>
#include "imgui/imgui.h"
#include "string/shine_string.h"

namespace shine::editor::widget {

namespace utils {
struct WidgetViewportTransform;
}

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

// Base component for all draggable editor widgets.
class WidgetComponent
{
public:
    virtual ~WidgetComponent() = default;

    int              id          = 0;
    EWidgetType      type        = EWidgetType::Button;
    SString          name        = "Widget";
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

    // Internal state
    bool             initialized = false;

    [[nodiscard]] virtual SString SerializeJson(bool prettify = true) const = 0;
    virtual bool DeserializeJson(STextView json) = 0;
    virtual void Draw(int* selectedId, bool isDesignMode, const utils::WidgetViewportTransform& transform) = 0;
};

// A single widget in the editor (like UWidget in UE5)
struct WidgetItem final : WidgetComponent
{
    using Base = WidgetComponent;

    SString      label       = "Label";
    SString      textValue   = SString{};
    bool         boolValue   = false;
    float        floatValue  = 0.0f;
    int          intValue    = 0;
    float        floatVec[4] = {0, 0, 0, 0};
    float        progress    = 0.5f;
    float        minValue    = 0.0f;
    float        maxValue    = 1.0f;
    float        itemWidth   = 200.0f;

    // Children (for panel widgets like CanvasPanel)
    std::vector<WidgetItem> children;

    void Draw(int* selectedId, bool isDesignMode, const utils::WidgetViewportTransform& transform) override;
    [[nodiscard]] SString SerializeJson(bool prettify = true) const override;
    bool DeserializeJson(STextView json) override;
    void Delete() { alive = false; }

private:
    void DrawHighlight(int* selectedId);
    void HandleDrag(int* selectedId, const utils::WidgetViewportTransform& transform);
};

// Canvas that holds all top-level widgets
struct WidgetCanvas
{
    bool                    active       = true;
    ImVec2                  canvasSize   = ImVec2(1280, 720);
    bool                    portraitMode = false;
    ImVec2                  canvasPos    = {};
    int                     idGen        = 0;
    int                     selectedId   = -1;
    std::vector<WidgetItem> widgets;

    [[nodiscard]] ImVec2 GetResolvedCanvasSize() const;
    void DrawAll(const utils::WidgetViewportTransform& transform);
    void Create(EWidgetType type, bool atCursor = false, const utils::WidgetViewportTransform* transform = nullptr);
    WidgetItem* FindWidget(int id);
    void DeleteWidget(int id);
    void Clear();
    [[nodiscard]] SString SaveToJson(bool prettify = true) const;
    bool LoadFromJson(STextView json);
};

} // namespace shine::editor::widget
