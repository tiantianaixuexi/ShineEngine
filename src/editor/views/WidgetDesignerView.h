#pragma once

#include "editor/views/BaseView.h"
#include "editor/widget/WidgetDesigner.h"

namespace shine::editor::views {

class WidgetDesignerView : public BaseView
{
public:
    WidgetDesignerView() = default;
    ~WidgetDesignerView() override = default;

    void onInit() override    { SetName("Widget Designer"); }
    void onRender() override  {
        ImGui::SetNextWindowSize(ImVec2(1200, 800), ImGuiCond_FirstUseEver);
        ImGui::Begin(name.c_str(), &isOpen);
        designer_.Render();
        ImGui::End();
    }
    void onShutDown() override {}

    widget::WidgetDesigner& GetDesigner() { return designer_; }

private:
    widget::WidgetDesigner designer_;
};

} // namespace shine::editor::views
