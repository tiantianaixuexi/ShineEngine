#pragma once

#include "BaseView.h"

namespace shine::editor::settings {
struct EngineSettings;
}

namespace shine::editor::views {

class SettingsView : public BaseView {
public:
 
    virtual ~SettingsView() = default;

    void FirstOpen() override;
    void onRender() override;
    void onInit() override;
    void onShutDown() override;


private:
    shine::editor::settings::EngineSettings *settings_;
};
} // namespace shine::editor::views
