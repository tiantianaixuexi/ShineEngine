#pragma once

#include "EngineCore/log/LogSystem.h"
#include "editor/views/BaseView.h"

namespace shine::editor::views {
class LogUI : public BaseView {
public:
    void onInit() override;
    void onRender() override;
    void onShutDown() override;

    void ClearLog();

private:
    bool m_AutoScroll = true;
};
} // namespace shine::editor::views
