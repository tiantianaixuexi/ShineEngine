#include "SettingsView.h"
#include "../settings/EngineSettings.h"
#include "../util/InspectorBuilder.h"
#include "../util/StaticInspector.h" // Include Static Inspector
#include "imgui/imgui.h"


namespace shine::editor::views {

void SettingsView::onInit() {
    SetName("引擎设置");
    SetShow();
}

void SettingsView::FirstOpen() {
    settings_ = new settings::EngineSettings();
}

void SettingsView::onRender() {

    ImGui::Begin(name.c_str(), &isOpen);
    util::StaticInspectorBuilder<settings::EngineSettings>::Draw(settings_);
    ImGui::End();

    CheckIsOpenChange();
}

void SettingsView::onShutDown() {
    if (settings_ != nullptr) {
        delete settings_;
        settings_ = nullptr;
    }
}

} // namespace shine::editor::views
