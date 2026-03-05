#pragma once

#include "editor/views/BaseView.h"

namespace shine::editor::views
{
    class PlacementPaletteView : public BaseView
    {
    public:
        void onInit() override;
        void onRender() override;
        void onShutDown() override;
    };
}
