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

class MainEditor : public IMainEditorCommands {

public:
    MainEditor(shine::EngineContext &context);
    ~MainEditor() override;

    bool mainDocker = true;

    void Init();
    void Render();

    bool ToggleAssetBrowser() override;
    bool ToggleMemoryProfiler() override;
	bool ToggleEngineSettings() override;
    bool ToggleLog() override;
    bool ToggleSceneHierarchy() override;
    bool TogglePlacementPalette() override;
    bool IsMemoryProfilerOpen() const override;

private:
    shine::EngineContext &m_Context;
    std::unique_ptr<WindowRegistry> windowRegistry_;
};

} // namespace shine::editor::main_editor
