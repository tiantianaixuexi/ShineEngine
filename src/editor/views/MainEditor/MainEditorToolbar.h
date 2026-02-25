#pragma once

#include "editor/views/BaseView.h"


namespace shine::editor::main_editor {
class MainEditor;
}
namespace shine::editor::views {
class SMainEditorToolbar : public BaseView {

public:
    SMainEditorToolbar(main_editor::MainEditor *_editor);
    virtual ~SMainEditorToolbar() = default;

    void onInit() override;
    void onRender() override;
    void onShutDown() override;


	bool                     AssetBorderShow    = false;
    bool                     MemoryProfilerShow = false;
    bool                     EngineSettingsShow = false;

private:

    main_editor::MainEditor *_mainEditor        = nullptr;
};

} // namespace shine::editor::views
