#pragma once

#include <memory>

// 前向声明
namespace shine::image
{
    class STexture;
}

namespace shine::editor::views
{
    /**
     * @brief 图片查看器窗口 - 显示图片纹理
     * 直接使用 STexture 资源类，简化使用
     */
    class ImageViewerView
    {
    public:
        ImageViewerView();
        ~ImageViewerView();

        void Render();
        
        /**
         * @brief 设置纹理资源（直接使用 STexture，更简单）
         * @param texture STexture 资源共享指针（使用 shared_ptr 确保生命周期安全）
         */
        void SetTexture(std::shared_ptr<shine::image::STexture> texture);
        
        void ClearTexture();

    private:
        std::shared_ptr<shine::image::STexture> texture_;  // 使用共享指针管理生命周期
        bool isOpen_ = true;
    };
}

