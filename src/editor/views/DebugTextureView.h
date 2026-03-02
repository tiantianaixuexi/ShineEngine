#pragma once

#include "BaseView.h"
#include <string>

namespace shine::editor::views
{
    class DebugTextureView : public BaseView
    {
    public:
        DebugTextureView() = default;
        ~DebugTextureView() override = default;

        void onInit() override;
        void onRender() override;
        void onShutDown() override;

    private:
        float m_TileSize = 160.0f;
        int m_Columns = 3;
        bool m_ShowDetail = false;
        std::string m_Selected;
        float m_DetailZoom = 1.0f;
        int m_ChannelMode = 0;
    };
}
