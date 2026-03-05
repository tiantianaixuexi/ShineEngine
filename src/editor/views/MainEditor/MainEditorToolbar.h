#pragma once

#include "editor/views/BaseView.h"
#include "editor/main_editor/editor_commands.h"


namespace shine::editor::views {
class SMainEditorToolbar : public BaseView {

public:
    SMainEditorToolbar(main_editor::IMainEditorCommands* commands);
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

private:

    main_editor::IMainEditorCommands* commands_ = nullptr;
};

} // namespace shine::editor::views
