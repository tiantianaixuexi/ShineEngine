#include "editor/main_editor/window_registry.h"

#include "fmt/format.h"
#include "imgui/imgui.h"

#include "EngineCore/engine_context.h"
#include "editor/browers/AssetsBrower.h"
#include "editor/log/LogUI.h"
#include "editor/main_editor/editor_commands.h"
#include "editor/views/DebugTextureView.h"
#include "editor/views/EditorView.h"
#include "editor/views/ImageViewerView.h"
#include "editor/views/MainEditor/MainEditorToolbar.h"
#include "editor/views/Profiler/MemoryProfiler.h"
#include "editor/views/PropertiesView.h"
#include "editor/views/SceneHierarchyView.h"
#include "editor/views/SettingsView.h"
#include "editor/views/placement/PlacementPaletteView.h"
#include "gameplay/world/WorldServiceLocator.h"
#include "render/demo/EngineDemoScene.h"
#include "widget/shineButton.h"

namespace shine::editor::main_editor
{
    namespace
    {
        ImGuiDockNodeFlags mainDockNodeFlags = ImGuiDockNodeFlags_None;
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    WindowRegistry::WindowRegistry() = default;
    WindowRegistry::~WindowRegistry() = default;

    void WindowRegistry::Init(IMainEditorCommands* commands)
    {
        auto& ctx = EngineContext::Get();
        auto* worldPlacementService = gameplay::world::ResolveWorldPlacementService(ctx);
        auto* worldHierarchyService = gameplay::world::ResolveWorldHierarchyService(ctx);

        mainEditorToolbar_ = std::make_unique<views::SMainEditorToolbar>(commands);
        const bool toolbarShown = mainEditorToolbar_->SetShow();
        (void)toolbarShown;

        myButton_ = std::make_unique<widget::button::shineButton>("应用编辑");
        myButton_->SetOnPressed([]() { fmt::println("应用编辑按钮被按下"); });
        myButton_->SetOnReleased([]() { fmt::println("应用编辑按钮被释放"); });
        myButton_->SetOnHovered([]() { fmt::println("应用编辑按钮被悬停"); });
        myButton_->SetOnUnHovered([]() { fmt::println("应用编辑按钮停止"); });

        memoryProfiler_ = std::make_unique<views::MemoryProfiler>();
        const auto memoryBindHandle = memoryProfiler_->OnOpenChange.bind([this](bool isOpen) {
            mainEditorToolbar_->MemoryProfilerShow = isOpen;
        });
        (void)memoryBindHandle;
        memoryProfiler_->onInit();

        assetsBrower_ = std::make_unique<assets_brower::AssetsBrower>();
        const auto assetsBindHandle = assetsBrower_->OnOpenChange.bind([this](bool isOpen) {
            mainEditorToolbar_->AssetBorderShow = isOpen;
        });
        (void)assetsBindHandle;
        assetsBrower_->onInit();

        editorView_ = std::make_unique<views::EditView>();
        editorView_->SetWorldPlacementService(worldPlacementService);
        editorView_->onInit();
        const bool editorShown = editorView_->SetShow();
        (void)editorShown;

        sceneHierarchyView_ = std::make_unique<views::SceneHierarchyView>();
        sceneHierarchyView_->SetWorldHierarchyService(worldHierarchyService);
        sceneHierarchyView_->onInit();

        propertiesView_ = std::make_unique<views::PropertiesView>();
        propertiesView_->onInit();

        imageViewerView_ = std::make_unique<views::ImageViewerView>();

        settingsView_ = std::make_unique<views::SettingsView>();
        const auto settingsBindHandle = settingsView_->OnOpenChange.bind([this](bool isOpen) {
            mainEditorToolbar_->EngineSettingsShow = isOpen;
        });
        (void)settingsBindHandle;
        settingsView_->onInit();

        logUI_ = std::make_unique<views::LogUI>();
        logUI_->onInit();

        debugTextureView_ = std::make_unique<views::DebugTextureView>();
        debugTextureView_->onInit();
        const bool debugShown = debugTextureView_->SetShow();
        (void)debugShown;

        placementPaletteView_ = std::make_unique<views::PlacementPaletteView>();
        const auto placementBindHandle = placementPaletteView_->OnOpenChange.bind([this](bool isOpen) {
            mainEditorToolbar_->PlacementPaletteShow = isOpen;
        });
        (void)placementBindHandle;
        placementPaletteView_->onInit();
    }

    void WindowRegistry::Render(bool& mainDocker)
    {
        static bool showWindows = true;
        ImGui::ShowDemoWindow(&showWindows);

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImGui::Begin("DockSpace Demo", &mainDocker, windowFlags);
        ImGui::PopStyleVar(2);
        ImGuiID mainDockId = ImGui::GetID("Engine Main Dock");
        ImGui::DockSpace(mainDockId, ImVec2(0.0f, 0.0f), mainDockNodeFlags);
        ImGui::End();

        mainEditorToolbar_->RenderBase();
        assetsBrower_->RenderBase();
        editorView_->RenderBase();

        sceneHierarchyView_->RenderBase();
        propertiesView_->SetSelectedObject(sceneHierarchyView_->GetSelectedObject());
        propertiesView_->RenderBase();
        imageViewerView_->Render();
        settingsView_->RenderBase();
        memoryProfiler_->RenderBase();
        logUI_->RenderBase();
        debugTextureView_->RenderBase();
        placementPaletteView_->RenderBase();

        ImGui::Render();
    }

    bool WindowRegistry::ToggleAssetBrowser()
    {
        return assetsBrower_->SetShow();
    }

    bool WindowRegistry::ToggleMemoryProfiler()
    {
        return memoryProfiler_->SetShow();
    }

    bool WindowRegistry::ToggleEngineSettings()
    {
        return settingsView_->SetShow();
    }

    bool WindowRegistry::ToggleLog()
    {
        return logUI_->SetShow();
    }

    bool WindowRegistry::ToggleSceneHierarchy()
    {
        return sceneHierarchyView_->SetShow();
    }

    bool WindowRegistry::TogglePlacementPalette()
    {
        return placementPaletteView_->SetShow();
    }

    bool WindowRegistry::IsMemoryProfilerOpen() const
    {
        return memoryProfiler_ ? memoryProfiler_->IsOpen() : false;
    }
}
