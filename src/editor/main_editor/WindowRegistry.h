#pragma once

#include <memory>

namespace shine::editor::main_editor
{
    class MainEditor;
}

namespace shine::editor::views
{
    class EditView;
    class SceneHierarchyView;
    class PropertiesView;
    class ImageViewerView;
    class SettingsView;
    class MemoryProfiler;
    class SMainEditorToolbar;
    class DebugTextureView;
    class LogUI;
    class PlacementPaletteView;
    class WidgetDesignerView;
}

namespace shine::widget::button
{
    class shineButton;
}

namespace shine::editor::assets_brower
{
    class AssetsBrower;
}

namespace shine::editor::main_editor
{
    class WindowRegistry
    {
    public:
        WindowRegistry();
        ~WindowRegistry();

        void Init(MainEditor* commands);
        void Render(bool& mainDocker);

        bool ToggleAssetBrowser();
        bool ToggleMemoryProfiler();
        bool ToggleEngineSettings();
        bool ToggleLog();
        bool ToggleSceneHierarchy();
        bool TogglePlacementPalette();
        bool ToggleDebugTexture();
        bool TogglePropertyInspector();
        bool ToggleWidgetDesigner();
        bool IsMemoryProfilerOpen() const; 


    private:
        std::unique_ptr<assets_brower::AssetsBrower> assetsBrower_;
        std::unique_ptr<views::EditView> editorView_;
        std::unique_ptr<views::SMainEditorToolbar> mainEditorToolbar_;
        std::unique_ptr<views::SceneHierarchyView> sceneHierarchyView_;
        std::unique_ptr<views::PropertiesView> propertiesView_;
        std::unique_ptr<views::ImageViewerView> imageViewerView_;
        std::unique_ptr<views::SettingsView> settingsView_;
        std::unique_ptr<views::MemoryProfiler> memoryProfiler_;
        std::unique_ptr<views::LogUI> logUI_;
        std::unique_ptr<views::DebugTextureView> debugTextureView_;
        std::unique_ptr<views::PlacementPaletteView> placementPaletteView_;
        std::unique_ptr<views::WidgetDesignerView> widgetDesignerView_;
        std::unique_ptr<widget::button::shineButton> myButton_;
    };
}
