#pragma once

#include "editor/views/BaseView.h"
#include "editor/main_editor/editor_commands.h"

namespace shine::editor::main_editor
{
    class MainEditor;
}

namespace shine::editor::views {
class SMainEditorToolbar : public BaseView {

public:
    SMainEditorToolbar(main_editor::MainEditor* commands);
    virtual ~SMainEditorToolbar() = default;

    void onInit() override;
    void onRender() override;
    void onShutDown() override;


	bool                     AssetBorderShow    = false;
    bool                     MemoryProfilerShow = false;
    bool                     EngineSettingsShow = false;
    bool                     LogShow            = false;
    bool                     SceneHierarchyShow = false;
    bool                     PlacementPaletteShow = false;
    bool                     DebugTextureShow     = false;
    bool                     PropertyInspectorShow = false;

private:

    main_editor::MainEditor* commands_ = nullptr;
};

} // namespace shine::editor::views
