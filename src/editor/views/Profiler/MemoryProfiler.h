#pragma once

#include <vector>

#include "editor/views/BaseView.h"

namespace shine::editor::views
{
    /**
     * @brief 内存监控面板 - 显示各系统的内存使用情况
     */
    class MemoryProfiler : public BaseView
    {
    public:
        
        virtual ~MemoryProfiler(){};

        void onInit()  override;
        void onShutDown() override;
        void onRender() override;
        


    private:

        bool m_PauseProfiling = false;
    };
}
