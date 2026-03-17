#pragma once

#include "EngineCore/log/LogSystem.h"
#include "editor/main_editor/editor_commands.h"

#include <memory>

namespace shine
{
    class EngineContext;
}

namespace shine::editor::main_editor
{
    class WindowRegistry;
}

namespace shine::editor::main_editor {

REGISTER_LOG_GROUP(EditorLog)

class MainEditor : public MainEditorCommandsCRTP<MainEditor> {

public:
    MainEditor(shine::EngineContext &context);
    ~MainEditor();

    bool mainDocker = true;

    void Init();
    void Render();

    bool ToggleAssetBrowserImpl();
    bool ToggleMemoryProfilerImpl();
	bool ToggleEngineSettingsImpl();
    bool ToggleLogImpl();
    bool ToggleSceneHierarchyImpl();
    bool TogglePlacementPaletteImpl();
    bool ToggleDebugTextureImpl();
    bool TogglePropertyInspectorImpl();
    bool ToggleWidgetDesignerImpl();
    bool IsMemoryProfilerOpenImpl() const;

    bool ToggleAssetBrowser()
    {
        return MainEditorCommandsCRTP<MainEditor>::ToggleAssetBrowser();
    }

    bool ToggleMemoryProfiler()
    {
        return MainEditorCommandsCRTP<MainEditor>::ToggleMemoryProfiler();
    }

    bool ToggleEngineSettings()
    {
        return MainEditorCommandsCRTP<MainEditor>::ToggleEngineSettings();
    }

    bool ToggleLog()
    {
        return MainEditorCommandsCRTP<MainEditor>::ToggleLog();
    }

    bool ToggleSceneHierarchy()
    {
        return MainEditorCommandsCRTP<MainEditor>::ToggleSceneHierarchy();
    }

    bool TogglePlacementPalette()
    {
        return MainEditorCommandsCRTP<MainEditor>::TogglePlacementPalette();
    }

    bool ToggleDebugTexture()
    {
        return MainEditorCommandsCRTP<MainEditor>::ToggleDebugTexture();
    }

    bool TogglePropertyInspector()
    {
        return MainEditorCommandsCRTP<MainEditor>::TogglePropertyInspector();
    }

    bool ToggleWidgetDesigner()
    {
        return MainEditorCommandsCRTP<MainEditor>::ToggleWidgetDesigner();
    }

    bool IsMemoryProfilerOpen() const
    {
        return MainEditorCommandsCRTP<MainEditor>::IsMemoryProfilerOpen();
    }



private:
    shine::EngineContext &m_Context;
    std::unique_ptr<WindowRegistry> windowRegistry_;
};

} // namespace shine::editor::main_editor
