#pragma once

namespace shine::editor::views
{
    /**
     * @brief 内存监控面板 - 显示各系统的内存使用情况
     */
    class MemoryProfiler
    {
    public:
        MemoryProfiler();
        ~MemoryProfiler();

        void Render();
        
        bool& IsOpen() { return isOpen_; }

    private:
        bool isOpen_ = true;
    };
}
