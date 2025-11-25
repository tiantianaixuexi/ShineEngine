#include "ImageViewerView.h"
#include "imgui.h"
#include "image/Texture.h"

namespace shine::editor::views
{
    ImageViewerView::ImageViewerView()
    {
    }

    ImageViewerView::~ImageViewerView()
    {
    }

    void ImageViewerView::Render()
    {
        if (!texture_ || !texture_->isValid())
        {
            return;
        }

        // 确保已创建 GPU 纹理资源
        if (!texture_->hasRenderResource())
        {
            texture_->CreateRenderResource();
        }

        // 直接从 texture_ 获取所有需要的信息
        u32 width = texture_->getWidth();
        u32 height = texture_->getHeight();
        uint32_t textureId = texture_->getTextureId();
        
        // 统一检查：如果尺寸或纹理ID无效，不显示
        if (width == 0 || height == 0 || textureId == 0)
        {
            return;
        }

        ImGui::Begin("图片查看器", &isOpen_);
        
        ImGui::Text("图片尺寸: %u x %u", width, height);
        ImGui::Separator();

        // 计算显示尺寸（保持宽高比，最大宽度800）
        float maxWidth = 800.0f;
        float aspectRatio = static_cast<float>(height) / static_cast<float>(width);
        float displayWidth = maxWidth;
        float displayHeight = maxWidth * aspectRatio;

        // 如果高度太大，限制高度
        if (displayHeight > 600.0f)
        {
            displayHeight = 600.0f;
            displayWidth = displayHeight / aspectRatio;
        }

        // 显示图像（直接从 texture_ 获取的纹理ID）
        ImGui::Image((ImTextureID)(intptr_t)textureId,
            ImVec2(displayWidth, displayHeight));

        ImGui::End();
    }

    void ImageViewerView::SetTexture(std::shared_ptr<shine::image::STexture> texture)
    {
        texture_ = texture;
    }

    void ImageViewerView::ClearTexture()
    {
        texture_.reset();
    }
}

