#include "ImageViewerView.h"
#include "imgui.h"
#include "image/Texture.h"
#include <algorithm>

using namespace shine::image;

namespace shine::editor::views
{
    ImageViewerView::ImageViewerView()
        : isOpen_(true)
        , zoom_(1.0f)
        , panOffset_(0.0f, 0.0f)
        , fitToWindow_(true)
        , isDragging_(false)
        , lastMousePos_(0.0f, 0.0f)
        , channelMode_(ChannelMode::RGBA)
        , desaturate_(false)
        , brightness_(0.0f)
        , contrast_(1.0f)
        , saturation_(1.0f)
        , hueShift_(0.0f, 0.0f)
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

        // 图片信息
        ImGui::Text("图片尺寸: %u x %u", width, height);
        ImGui::SameLine();
        ImGui::Text("缩放: %.1f%%", zoom_ * 100.0f);

        // 详细图片信息
        if (ImGui::CollapsingHeader("详细信息", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Columns(2, "image_details", false);

            // 左侧列
            ImGui::Text("宽度: %u 像素", width);
            ImGui::Text("高度: %u 像素", height);
            ImGui::Text("纵横比: %.3f", static_cast<float>(width) / static_cast<float>(height));
            ImGui::Text("像素总数: %u", width * height);

            ImGui::NextColumn();

            // 右侧列
            ImGui::Text("数据大小: %.2f KB", static_cast<float>(texture_->getDataSize() * sizeof(shine::image::RGBA8)) / 1024.0f);
            ImGui::Text("纹理类型: %s", texture_->getType() == TextureType::TEXTURE_2D ? "2D" : "其他");

            // 格式信息
            const char* formatStr = "未知";
            auto format = texture_->getFormat();
            switch (format)
            {
            case shine::image::TextureFormat::R: formatStr = "R (单通道)"; break;
            case shine::image::TextureFormat::RG: formatStr = "RG (双通道)"; break;
            case shine::image::TextureFormat::RGB: formatStr = "RGB (三通道)"; break;
            case shine::image::TextureFormat::RGBA: formatStr = "RGBA (四通道)"; break;
            case shine::image::TextureFormat::BC1_RGB: formatStr = "BC1_RGB (DXT1)"; break;
            case shine::image::TextureFormat::BC1_RGBA: formatStr = "BC1_RGBA (DXT1)"; break;
            case shine::image::TextureFormat::BC3_RGBA: formatStr = "BC3_RGBA (DXT5)"; break;
            case shine::image::TextureFormat::BC7_RGBA: formatStr = "BC7_RGBA"; break;
            default: formatStr = "压缩格式"; break;
            }
            ImGui::Text("格式: %s", formatStr);

            ImGui::Columns(1);
        }
        ImGui::Separator();

        // 工具栏
        if (ImGui::Button("适应窗口"))
        {
            FitToWindow();
        }
        ImGui::SameLine();
        if (ImGui::Button("实际大小"))
        {
            ZoomToActualSize();
        }
        ImGui::SameLine();
        if (ImGui::Button("放大"))
        {
            SetZoom(zoom_ * 1.2f);
        }
        ImGui::SameLine();
        if (ImGui::Button("缩小"))
        {
            SetZoom(zoom_ / 1.2f);
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        if (ImGui::SliderFloat("缩放%", &zoom_, 0.01f, 10.0f, "%.1f%%"))
        {
            fitToWindow_ = false;
            panOffset_.Set(0, 0);
        }

        // 图像处理控制
        ImGui::Separator();
        if (ImGui::CollapsingHeader("图像处理", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // 通道预览
            const char* channelNames[] = {"RGBA", "R", "G", "B", "A"};
            int currentChannel = static_cast<int>(channelMode_);
            if (ImGui::Combo("通道预览", &currentChannel, channelNames, IM_ARRAYSIZE(channelNames)))
            {
                channelMode_ = static_cast<ChannelMode>(currentChannel);
            }

            ImGui::Checkbox("去色", &desaturate_);

            // 颜色调整
            ImGui::SliderFloat("亮度", &brightness_, -1.0f, 1.0f);
            ImGui::SliderFloat("对比度", &contrast_, 0.0f, 2.0f);
            ImGui::SliderFloat("饱和度", &saturation_, 0.0f, 2.0f);

            if (ImGui::Button("重置调整"))
            {
                brightness_ = 0.0f;
                contrast_ = 1.0f;
                saturation_ = 1.0f;
                hueShift_.Set(0.0f, 0.0f);
            }
        }

        ImGui::Separator();

        // 显示图像
        RenderImageWithEffects();


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

    void ImageViewerView::FitToWindow()
    {
        fitToWindow_ = true;
        panOffset_.Set(0, 0);
    }

    void ImageViewerView::ZoomToActualSize()
    {
        fitToWindow_ = false;
        zoom_ = 1.0f;
        panOffset_.Set(0, 0);
    }

    void ImageViewerView::SetZoom(float zoom)
    {
        fitToWindow_ = false;
        zoom_ = (zoom < 0.01f) ? 0.01f : ((zoom > 10.0f) ? 10.0f : zoom); // 限制缩放范围在0.01到10之间
        panOffset_.Set(0, 0); // 重置平移
    }

    void ImageViewerView::RenderImageWithEffects()
    {
        if (!texture_ || !texture_->isValid())
            return;

        // 获取图片尺寸
        u32 width = texture_->getWidth();
        u32 height = texture_->getHeight();
        uint32_t textureId = texture_->getTextureId();

        if (width == 0 || height == 0 || textureId == 0)
            return;

        // 计算显示尺寸
        float displayWidth, displayHeight;
        if (fitToWindow_)
        {
            ImVec2 availSize = ImGui::GetContentRegionAvail();
            float windowAspect = availSize.x / availSize.y;
            float imageAspect = static_cast<float>(width) / static_cast<float>(height);

            if (imageAspect > windowAspect)
            {
                displayWidth = availSize.x;
                displayHeight = displayWidth / imageAspect;
            }
            else
            {
                displayHeight = availSize.y;
                displayWidth = displayHeight * imageAspect;
            }
            zoom_ = displayWidth / width;
        }
        else
        {
            displayWidth = static_cast<float>(width) * zoom_;
            displayHeight = static_cast<float>(height) * zoom_;
        }

        // 创建可滚动的子窗口
        ImVec2 childSize = ImGui::GetContentRegionAvail();
        float minChildWidth = displayWidth;
        float minChildHeight = displayHeight;

        if (panOffset_.X != 0.0f)
        {
            minChildWidth += std::abs(panOffset_.X);
        }
        if (panOffset_.Y != 0.0f)
        {
            minChildHeight += std::abs(panOffset_.Y);
        }

        childSize.x = (childSize.x > minChildWidth) ? childSize.x : minChildWidth;
        childSize.y = (childSize.y > minChildHeight) ? childSize.y : minChildHeight;

        ImGui::BeginChild("ImageViewerChild", childSize, false,
            ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar |
            ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);

        // 计算图片显示位置（考虑平移）
        ImVec2 imagePos = ImGui::GetCursorScreenPos();
        if (panOffset_.X != 0.0f || panOffset_.Y != 0.0f)
        {
            imagePos.x += panOffset_.X;
            imagePos.y += panOffset_.Y;
            ImGui::SetCursorScreenPos(imagePos);
        }

        // 如果有图像处理效果，需要创建自定义的纹理
        if (channelMode_ != ChannelMode::RGBA || desaturate_ ||
            brightness_ != 0.0f || contrast_ != 1.0f ||
            saturation_ != 1.0f || hueShift_.X != 0.0f)
        {
            // 创建处理后的图像数据
            const auto& originalData = texture_->getData();
            std::vector<shine::image::RGBA8> processedData = originalData;

            for (size_t i = 0; i < processedData.size(); ++i)
            {
                shine::math::FVector2f rgba(
                    processedData[i].r / 255.0f,
                    processedData[i].g / 255.0f
                );
                shine::math::FVector2f bgra(
                    processedData[i].b / 255.0f,
                    processedData[i].a / 255.0f
                );

                // 应用通道选择
                float channelValue = GetChannelValue(channelMode_, rgba.X, rgba.Y, bgra.X, bgra.Y);

                // 应用颜色调整
                shine::math::FVector2f adjustedColor = ApplyColorAdjustments(shine::math::FVector2f(rgba.X, rgba.Y));

                // 转换为灰度（如果去色）
                if (desaturate_)
                {
                    float gray = 0.299f * adjustedColor.X + 0.587f * adjustedColor.Y + 0.114f * bgra.X;
                    adjustedColor.X = adjustedColor.Y = bgra.X = gray;
                }

                // 应用通道模式
                if (channelMode_ != ChannelMode::RGBA)
                {
                    adjustedColor.X = adjustedColor.Y = bgra.X = channelValue;
                }

                processedData[i].r = static_cast<u8>((adjustedColor.X < 0.0f) ? 0.0f : ((adjustedColor.X > 1.0f) ? 255.0f : (adjustedColor.X * 255.0f)));
                processedData[i].g = static_cast<u8>((adjustedColor.Y < 0.0f) ? 0.0f : ((adjustedColor.Y > 1.0f) ? 255.0f : (adjustedColor.Y * 255.0f)));
                processedData[i].b = static_cast<u8>((bgra.X < 0.0f) ? 0.0f : ((bgra.X > 1.0f) ? 255.0f : (bgra.X * 255.0f)));
                processedData[i].a = static_cast<u8>((bgra.Y < 0.0f) ? 0.0f : ((bgra.Y > 1.0f) ? 255.0f : (bgra.Y * 255.0f)));
            }

            // TODO: 创建临时纹理并显示处理后的图像
            // 这里需要实现纹理上传到GPU的功能
            // 暂时显示原始图像
            ImGui::Image((ImTextureID)(intptr_t)textureId, ImVec2(displayWidth, displayHeight));
        }
        else
        {
            // 显示原始图像
            ImGui::Image((ImTextureID)(intptr_t)textureId, ImVec2(displayWidth, displayHeight));
        }

        // 鼠标交互处理
        bool isImageHovered = ImGui::IsItemHovered();

        // 处理像素检查器和缩放
        if (isImageHovered)
        {
            // 像素检查器
            ImVec2 mousePos = ImGui::GetMousePos();
            ImVec2 relativePos = ImVec2(mousePos.x - imagePos.x, mousePos.y - imagePos.y);

            // 计算对应的像素坐标
            int pixelX = static_cast<int>(relativePos.x / displayWidth * width);
            int pixelY = static_cast<int>(relativePos.y / displayHeight * height);

            // 确保像素坐标在有效范围内
            if (pixelX >= 0 && pixelX < static_cast<int>(width) &&
                pixelY >= 0 && pixelY < static_cast<int>(height))
            {
                // 获取像素数据
                const auto& data = texture_->getData();
                size_t pixelIndex = pixelY * width + pixelX;
                if (pixelIndex < data.size())
                {
                    const auto& pixel = data[pixelIndex];

                    // 显示像素信息工具提示
                    ImGui::BeginTooltip();
                    ImGui::Text("Pixel Position: (%d, %d)", pixelX, pixelY);
                    ImGui::Separator();

                    // RGB值
                    ImGui::Text("R: %d, G: %d, B: %d, A: %d", pixel.r, pixel.g, pixel.b, pixel.a);

                    // 归一化值
                    ImGui::Text("R: %.3f, G: %.3f, B: %.3f, A: %.3f",
                        pixel.r / 255.0f, pixel.g / 255.0f, pixel.b / 255.0f, pixel.a / 255.0f);

                    // 十六进制
                    ImGui::Text("Hex: #%02X%02X%02X%02X", pixel.r, pixel.g, pixel.b, pixel.a);

                    // 颜色预览
                    ImVec4 color(pixel.r / 255.0f, pixel.g / 255.0f, pixel.b / 255.0f, pixel.a / 255.0f);
                    ImGui::ColorButton("##pixel_color", color, ImGuiColorEditFlags_NoTooltip, ImVec2(40, 20));
                    ImGui::SameLine();
                    ImGui::Text("Color Preview");

                    ImGui::EndTooltip();
                }
            }

            // 鼠标滚轮缩放
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f)
            {
                fitToWindow_ = false;
                ImVec2 imageCenter = ImVec2(imagePos.x + displayWidth * 0.5f, imagePos.y + displayHeight * 0.5f);

                // 计算鼠标相对于图片中心的偏移
                ImVec2 offset = ImVec2(mousePos.x - imageCenter.x, mousePos.y - imageCenter.y);

                // 计算缩放前的实际像素位置
                ImVec2 pixelPos = ImVec2(
                    (offset.x / zoom_) / displayWidth * width,
                    (offset.y / zoom_) / displayHeight * height
                );

                // 应用缩放
                float zoomFactor = (wheel > 0) ? 1.2f : 0.8f;
                float newZoom = zoom_ * zoomFactor;
                newZoom = (newZoom < 0.01f) ? 0.01f : ((newZoom > 10.0f) ? 10.0f : newZoom);

                // 调整平移以保持鼠标下的像素位置
                if (newZoom != zoom_)
                {
                    zoom_ = newZoom;
                    ImVec2 newOffset = ImVec2(
                        pixelPos.x * zoom_ * displayWidth / width,
                        pixelPos.y * zoom_ * displayHeight / height
                    );
                    panOffset_.Set(offset.x - newOffset.x, offset.y - newOffset.y);
                }
            }
        }

        // 处理拖拽平移
        if (ImGui::IsWindowHovered())
        {
            if ((ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
                 (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && ImGui::GetIO().KeyAlt)))
            {
                if (!isDragging_)
                {
                    isDragging_ = true;
                    ImVec2 mousePos = ImGui::GetMousePos();
                    lastMousePos_.Set(mousePos.x, mousePos.y);
                }
                else
                {
                    ImVec2 currentMousePos = ImGui::GetMousePos();
                    ImVec2 delta = ImVec2(currentMousePos.x - lastMousePos_.X, currentMousePos.y - lastMousePos_.Y);
                    panOffset_.Set(panOffset_.X + delta.x, panOffset_.Y + delta.y);
                    lastMousePos_.Set(currentMousePos.x, currentMousePos.y);
                    fitToWindow_ = false;
                }
            }
            else
            {
                isDragging_ = false;
            }
        }
        else
        {
            isDragging_ = false;
        }

        ImGui::EndChild();
    }

    shine::math::FVector2f ImageViewerView::ApplyColorAdjustments(const shine::math::FVector2f& color) const
    {
        // 这里简化实现，只处理r和g分量
        // 实际应该处理r,g,b,a四个分量
        shine::math::FVector2f result = color;

        // 亮度调整
        result.X += brightness_;
        result.Y += brightness_;

        // 对比度调整
        result.X = (result.X - 0.5f) * contrast_ + 0.5f;
        result.Y = (result.Y - 0.5f) * contrast_ + 0.5f;

        // 饱和度调整 (简化版本，基于当前r,g分量)
        if (saturation_ != 1.0f)
        {
            float gray = 0.299f * result.X + 0.587f * result.Y + 0.114f * 0.5f; // 假设b=0.5
            result.X = gray + (result.X - gray) * saturation_;
            result.Y = gray + (result.Y - gray) * saturation_;
        }

        // 限制在0-1范围内
        result.X = (result.X < 0.0f) ? 0.0f : ((result.X > 1.0f) ? 1.0f : result.X);
        result.Y = (result.Y < 0.0f) ? 0.0f : ((result.Y > 1.0f) ? 1.0f : result.Y);

        return result;
    }

    float ImageViewerView::GetChannelValue(ChannelMode mode, float r, float g, float b, float a) const
    {
        switch (mode)
        {
        case ChannelMode::R: return r;     // Red channel
        case ChannelMode::G: return g;     // Green channel
        case ChannelMode::B: return b;     // Blue channel
        case ChannelMode::A: return a;     // Alpha channel
        case ChannelMode::RGBA:
        default: return r;                 // Default to red channel
        }
    }
}

