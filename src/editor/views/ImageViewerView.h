#pragma once

#include <memory>
#include "math/vector2.h"

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
        bool isOpen_;

        // 缩放和显示相关
        float zoom_;                       // 当前缩放比例
        shine::math::FVector2f panOffset_; // 平移偏移
        bool fitToWindow_;                 // 是否适应窗口大小
        bool isDragging_;                  // 是否正在拖拽
        shine::math::FVector2f lastMousePos_; // 上次鼠标位置

        // 缩放控制方法
        void FitToWindow();
        void ZoomToActualSize();
        void SetZoom(float zoom);
        float GetZoom() const { return zoom_; }
    };
}

