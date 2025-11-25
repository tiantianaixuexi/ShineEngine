#pragma once

#include <memory>
#include "math/vector2.h"

// 前向声明
namespace shine::image
{
    class STexture;
}

// 通道预览模式
enum class ChannelMode
{
    RGBA,   // 完整RGBA
    R,      // 仅红色通道
    G,      // 仅绿色通道
    B,      // 仅蓝色通道
    A       // 仅透明通道
};

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

        // 图像处理相关
        ChannelMode channelMode_;          // 通道预览模式
        bool desaturate_;                  // 是否去色
        float brightness_;                 // 亮度调整 (-1.0 to 1.0)
        float contrast_;                   // 对比度调整 (0.0 to 2.0)
        float saturation_;                 // 饱和度调整 (0.0 to 2.0)
        shine::math::FVector2f hueShift_;  // 色相偏移 (x: 色相, y: 强度)

        // 缩放控制方法
        void FitToWindow();
        void ZoomToActualSize();
        void SetZoom(float zoom);
        float GetZoom() const { return zoom_; }

        // 图像处理方法
        void RenderImageWithEffects();
        shine::math::FVector2f ApplyColorAdjustments(const shine::math::FVector2f& color) const;
        float GetChannelValue(ChannelMode mode, float r, float g, float b, float a) const;
    };
}

