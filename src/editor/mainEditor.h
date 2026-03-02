#pragma once

#include "EngineCore/log/LogSystem.h"

#include "editor/browers/AssetsBrower.h"
#include "editor/log/LogUI.h"
#include "editor/views/EditorView.h"
#include "widget/shineButton.h"

// 前向声明
namespace shine::editor::views {
class SceneHierarchyView;
class PropertiesView;
class ImageViewerView;
class SettingsView;
class MemoryProfiler;
class SMainEditorToolbar;
class DebugTextureView;
} // namespace shine::editor::views

namespace shine::editor::main_editor {

REGISTER_LOG_GROUP(EditorLog)

using namespace widget;

class MainEditor {

public:
    MainEditor(shine::EngineContext &context);
    ~MainEditor();

    bool mainDocker = true;

    void Init();
    void Render();

    bool setAssetBorwerOpen();
    bool setMemoryProfilerOpen();
	bool setEngineSettingsOpen();
    bool getMemoryProfilerOpen() const;

private:
    shine::EngineContext &m_Context;

    assets_brower::AssetsBrower *assetsBrower = nullptr;
    views::EditView        *editorView   = nullptr;

    // 视图窗口
    views::SMainEditorToolbar *mainEditorToolbar  = nullptr;
    views::SceneHierarchyView *sceneHierarchyView = nullptr;
    views::PropertiesView     *propertiesView     = nullptr;
    views::ImageViewerView    *imageViewerView    = nullptr;
    views::SettingsView       *settingsView       = nullptr;
    views::MemoryProfiler     *memoryProfiler     = nullptr;
    views::LogUI              *logUI              = nullptr;
    views::DebugTextureView   *debugTextureView   = nullptr;

    button::shineButton *myButton = nullptr;
};

} // namespace shine::editor::main_editor
