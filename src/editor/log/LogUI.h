#pragma once
#include "EngineCore/log/LogSystem.h"

namespace shine::editor::views {
    class LogUI {
    public:
        LogUI();
        void Render();
        void Clear();
    private:
        bool m_AutoScroll = true;
    };
}
