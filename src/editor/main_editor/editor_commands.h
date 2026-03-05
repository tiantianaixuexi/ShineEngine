#pragma once

namespace shine::editor::main_editor
{
    class IMainEditorCommands
    {
    public:
        virtual ~IMainEditorCommands() = default;
        virtual bool ToggleAssetBrowser() = 0;
        virtual bool ToggleMemoryProfiler() = 0;
        virtual bool ToggleEngineSettings() = 0;
        virtual bool ToggleLog() = 0;
        virtual bool ToggleSceneHierarchy() = 0;
        virtual bool TogglePlacementPalette() = 0;
        virtual bool IsMemoryProfilerOpen() const = 0;
    };
}
