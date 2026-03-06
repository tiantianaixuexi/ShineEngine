#pragma once

namespace shine::editor::main_editor
{
    template <typename Derived>
    class MainEditorCommandsCRTP
    {
    public:
        bool ToggleAssetBrowser(this auto& self)
        {
            return self.ToggleAssetBrowser();
        }

        bool ToggleMemoryProfiler(this auto& self)
        {
            return self.ToggleMemoryProfilerImpl();
        }

        bool ToggleEngineSettings(this auto& self)
        {
            return self.ToggleEngineSettingsImpl();
        }

        bool ToggleLog(this auto& self)
        {
            return self.ToggleLogImpl();
        }

        bool ToggleSceneHierarchy(this auto& self)
        {
            return self.ToggleSceneHierarchyImpl();
        }

        bool TogglePlacementPalette(this auto& self)
        {
            return self.TogglePlacementPaletteImpl();
        }

        bool ToggleDebugTexture(this auto& self)
        {
            return self.ToggleDebugTextureImpl();
        }
        
        bool TogglePropertyInspector(this auto & self)
        {
            return self.TogglePropertyInspectorImpl();
        }

        bool IsMemoryProfilerOpen(this auto const& self)
        {
            return self.IsMemoryProfilerOpenImpl();
        }

    };
}
