#pragma once

namespace shine::editor::settings {
    struct EngineSettings;
}

namespace shine::editor::views {

    class SettingsView {
    public:
        SettingsView();
        ~SettingsView();

        void Render();

        bool isOpen_ = true;

    private:
        shine::editor::settings::EngineSettings* settings_;
    };
}
