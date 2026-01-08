#include "SettingsView.h"
#include "../settings/EngineSettings.h"
#include "imgui/imgui.h"
#include "../util/InspectorBuilder.h"
#include "../util/StaticInspector.h" // Include Static Inspector

namespace shine::editor::views {

    SettingsView::SettingsView() {
        settings_ = new settings::EngineSettings();

        // Ensure static reflection data is registered
        // The Header-only registration should work now because we reference it in EngineSettings ctor.
        // So we only need to flush pending types here.
        
        // CRITICAL: Must Flush Pending Types to Registry for Lookup to work!
        Shine::Reflection::TypeRegistry::RegisterAllTypes();
    }

    SettingsView::~SettingsView() {
        delete settings_;
    }

    void SettingsView::Render() {
        if (!isOpen_) return;

        ImGui::Begin("引擎设置", &isOpen_);

        // Option 1: Dynamic Runtime Reflection (Slower, but works for unknown types)
        // util::InspectorBuilder::DrawInspector(settings_);

        // Option 2: Static Compile-time Reflection (Super Fast, Optimized Assembly)
        util::StaticInspectorBuilder<settings::EngineSettings>::Draw(settings_);

        ImGui::End();
    }
}
