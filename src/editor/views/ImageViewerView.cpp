#include "ImageViewerView.h"
#include "imgui.h"
#include "image/Texture.h"
#include "render/resources/texture_manager.h"

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
            // 如果没有纹理，不显示窗口
            return;
        }

        // 确保已创建 GPU 纹理资源
        if (!texture_->hasRenderResource())
        {
            texture_->CreateRenderResource();
        }

        // 直接从 STexture 获取尺寸（不需要通过 TextureManager）
        u32 width = texture_->getWidth();
        u32 height = texture_->getHeight();
        
        if (width == 0 || height == 0)
        {
            return;
        }

        // 从 TextureManager 获取纹理ID（用于 ImGui 显示）
        auto& textureManager = shine::render::TextureManager::get();
        const auto& renderHandle = texture_->getRenderHandle();
        uint32_t textureId = textureManager.GetTextureId(renderHandle);
        
        if (textureId == 0)
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

        // 显示图像（将纹理ID转换为ImTextureID）
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

