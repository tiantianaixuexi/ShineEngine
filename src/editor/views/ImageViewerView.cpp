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
        , needsReprocessing_(true)
        , cachedChannelMode_(ChannelMode::RGBA)
        , cachedDesaturate_(false)
        , cachedBrightness_(0.0f)
        , cachedContrast_(1.0f)
        , cachedSaturation_(1.0f)
        , cachedHueShift_(0.0f, 0.0f)
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

        // 创建左右分割布局
        float leftPanelWidth = ImGui::GetContentRegionAvail().x * 0.7f; // 左侧占70%
        float rightPanelWidth = ImGui::GetContentRegionAvail().x * 0.3f; // 右侧占30%

        // 左侧：图像显示区域
        ImGui::BeginChild("ImageArea", ImVec2(leftPanelWidth, 0), true);

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
        if (ImGui::SliderFloat("缩放", &zoom_, 0.01f, 10.0f, "%.2f"))
        {
            fitToWindow_ = false;
            panOffset_.Set(0, 0);
        }
        ImGui::Separator();

        // 显示图像
        RenderImageWithEffects();

        ImGui::EndChild();

        // 右侧：控制面板
        ImGui::SameLine();
        ImGui::BeginChild("ControlPanel", ImVec2(rightPanelWidth, 0), true);

        ImGui::Text("图像处理控制");
        ImGui::Separator();

        // 通道预览
        const char* channelNames[] = {"RGBA", "R", "G", "B", "A"};
        int currentChannel = static_cast<int>(channelMode_);
        if (ImGui::Combo("通道预览", &currentChannel, channelNames, IM_ARRAYSIZE(channelNames)))
        {
            channelMode_ = static_cast<ChannelMode>(currentChannel);
            needsReprocessing_ = true;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("颜色调整");

        if (ImGui::Checkbox("去色", &desaturate_))
        {
            needsReprocessing_ = true;
        }

        if (ImGui::SliderFloat("亮度", &brightness_, -1.0f, 1.0f))
        {
            needsReprocessing_ = true;
        }

        if (ImGui::SliderFloat("对比度", &contrast_, 0.0f, 2.0f))
        {
            needsReprocessing_ = true;
        }

        if (ImGui::SliderFloat("饱和度", &saturation_, 0.0f, 2.0f))
        {
            needsReprocessing_ = true;
        }

        ImGui::Spacing();
        if (ImGui::Button("重置调整", ImVec2(-1, 0)))
        {
            brightness_ = 0.0f;
            contrast_ = 1.0f;
            saturation_ = 1.0f;
            hueShift_.Set(0.0f, 0.0f);
            desaturate_ = false;
            channelMode_ = ChannelMode::RGBA;
            needsReprocessing_ = true;
        }

        ImGui::EndChild();

        ImGui::End();
    }

    void ImageViewerView::SetTexture(std::shared_ptr<shine::image::STexture> texture)
    {
        texture_ = texture;
        processedTexture_.reset();
        needsReprocessing_ = true;

        // 重置缓存参数，确保下次处理时能正确比较
        cachedChannelMode_ = ChannelMode::RGBA;
        cachedDesaturate_ = false;
        cachedBrightness_ = 0.0f;
        cachedContrast_ = 1.0f;
        cachedSaturation_ = 1.0f;
        cachedHueShift_.Set(0.0f, 0.0f);
    }

    void ImageViewerView::ClearTexture()
    {
        texture_.reset();
        processedTexture_.reset();
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

    void ImageViewerView::UpdateProcessedTexture()
    {
        if (!texture_ || !texture_->isValid())
            return;

        // 检查处理参数是否发生变化
        bool paramsChanged = (channelMode_ != cachedChannelMode_ ||
                             desaturate_ != cachedDesaturate_ ||
                             brightness_ != cachedBrightness_ ||
                             contrast_ != cachedContrast_ ||
                             saturation_ != cachedSaturation_ ||
                             hueShift_.X != cachedHueShift_.X ||
                             hueShift_.Y != cachedHueShift_.Y);

        // 如果没有处理需求且参数没有变化，直接使用原始纹理
        if (!paramsChanged && channelMode_ == ChannelMode::RGBA && !desaturate_ &&
            brightness_ == 0.0f && contrast_ == 1.0f &&
            saturation_ == 1.0f && hueShift_.X == 0.0f)
        {
            // 不需要处理时，清空processedTexture_，在渲染时直接使用texture_
            processedTexture_.reset();
            needsReprocessing_ = false;
            return;
        }

        // 如果参数没有变化且已处理过，直接返回
        if (!paramsChanged && processedTexture_ && processedTexture_->hasRenderResource())
        {
            needsReprocessing_ = false;
            return;
        }

        // 参数发生变化，需要重新处理
        // 创建或更新处理后的纹理
        if (!processedTexture_)
        {
            processedTexture_ = std::make_shared<shine::image::STexture>();
        }

        // 复制原始纹理的基本信息
        processedTexture_->setWidth(texture_->getWidth());
        processedTexture_->setHeight(texture_->getHeight());
        processedTexture_->setFormat(texture_->getFormat());
        processedTexture_->setType(texture_->getType());

        // 获取原始数据并处理
        const auto& originalData = texture_->getData();
        std::vector<shine::image::RGBA8> processedData = originalData;

        for (size_t i = 0; i < processedData.size(); ++i)
        {
            // 归一化到0-1范围
            float r = processedData[i].r / 255.0f;
            float g = processedData[i].g / 255.0f;
            float b = processedData[i].b / 255.0f;
            float a = processedData[i].a / 255.0f;

            // 应用颜色调整
            ApplyColorAdjustments(r, g, b, a);

            // 转换为灰度（如果去色）
            if (desaturate_)
            {
                float gray = 0.299f * r + 0.587f * g + 0.114f * b;
                r = g = b = gray;
            }

            // 应用通道模式
            if (channelMode_ != ChannelMode::RGBA)
            {
                float channelValue = GetChannelValue(channelMode_, r, g, b, a);
                r = g = b = channelValue;
            }

            // 转换回0-255范围
            processedData[i].r = static_cast<u8>((r < 0.0f) ? 0.0f : ((r > 1.0f) ? 255.0f : (r * 255.0f)));
            processedData[i].g = static_cast<u8>((g < 0.0f) ? 0.0f : ((g > 1.0f) ? 255.0f : (g * 255.0f)));
            processedData[i].b = static_cast<u8>((b < 0.0f) ? 0.0f : ((b > 1.0f) ? 255.0f : (b * 255.0f)));
            processedData[i].a = static_cast<u8>((a < 0.0f) ? 0.0f : ((a > 1.0f) ? 255.0f : (a * 255.0f)));
        }

        // 更新纹理数据
        processedTexture_->setData(processedData);

        // 重新创建GPU资源以应用新数据
        if (processedTexture_->hasRenderResource())
        {
            processedTexture_->ReleaseRenderResource();
        }
        processedTexture_->CreateRenderResource();

        // 更新缓存的参数
        cachedChannelMode_ = channelMode_;
        cachedDesaturate_ = desaturate_;
        cachedBrightness_ = brightness_;
        cachedContrast_ = contrast_;
        cachedSaturation_ = saturation_;
        cachedHueShift_ = hueShift_;

        needsReprocessing_ = false;
    }

    void ImageViewerView::RenderImageWithEffects()
    {
        if (!texture_ || !texture_->isValid())
            return;

        // 检查是否需要重新处理
        if (needsReprocessing_)
        {
            UpdateProcessedTexture();
        }

        // 获取当前要显示的纹理
        auto currentTexture = processedTexture_ ? processedTexture_ : texture_;

        // 获取图片尺寸
        u32 width = currentTexture->getWidth();
        u32 height = currentTexture->getHeight();
        uint32_t textureId = currentTexture->getTextureId();

        if (width == 0 || height == 0 || textureId == 0)
            return;

        // 计算显示尺寸
        float displayWidth, displayHeight;
        if (fitToWindow_)
        {
            ImVec2 availSize = ImGui::GetContentRegionAvail();
            // 防止除零错误
            float windowAspect = (availSize.y > 0.0f) ? (availSize.x / availSize.y) : 1.0f;
            float imageAspect = (height > 0) ? (static_cast<float>(width) / static_cast<float>(height)) : 1.0f;

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

        // 显示处理后的图像
        ImGui::Image((ImTextureID)(intptr_t)textureId, ImVec2(displayWidth, displayHeight));

        // 鼠标交互处理
        bool isImageHovered = ImGui::IsItemHovered();

        // 处理像素检查器和缩放
        if (isImageHovered)
        {
            // 像素检查器
            ImVec2 mousePos = ImGui::GetMousePos();
            ImVec2 relativePos = ImVec2(mousePos.x - imagePos.x, mousePos.y - imagePos.y);

            // 计算对应的像素坐标（防止除零错误）
            int pixelX = 0;
            int pixelY = 0;
            if (displayWidth > 0.0f && displayHeight > 0.0f)
            {
                pixelX = static_cast<int>(relativePos.x / displayWidth * width);
                pixelY = static_cast<int>(relativePos.y / displayHeight * height);
            }

            // 确保像素坐标在有效范围内
            if (pixelX >= 0 && pixelX < static_cast<int>(width) &&
                pixelY >= 0 && pixelY < static_cast<int>(height))
            {
                // 获取处理后纹理的像素数据
                const auto& data = currentTexture->getData();
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

                // 计算缩放前的实际像素位置（相对于图片左上角的像素坐标）
                ImVec2 pixelPos = ImVec2(
                    (offset.x / zoom_) + width * 0.5f,   // 从中心偏移转换到像素坐标
                    (offset.y / zoom_) + height * 0.5f
                );

                // 应用缩放
                float zoomFactor = (wheel > 0) ? 1.2f : 0.8f;
                float newZoom = zoom_ * zoomFactor;
                newZoom = (newZoom < 0.01f) ? 0.01f : ((newZoom > 10.0f) ? 10.0f : newZoom);

                // 调整平移以保持鼠标下的像素位置
                if (newZoom != zoom_)
                {
                    zoom_ = newZoom;
                    // 计算新的中心偏移
                    ImVec2 newOffset = ImVec2(
                        (pixelPos.x - width * 0.5f) * newZoom,   // 从像素坐标转换回屏幕偏移
                        (pixelPos.y - height * 0.5f) * newZoom
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

    void ImageViewerView::ApplyColorAdjustments(float& r, float& g, float& b, float& a) const
    {
        // 亮度调整
        if (brightness_ != 0.0f)
        {
            r += brightness_;
            g += brightness_;
            b += brightness_;
        }

        // 对比度调整
        if (contrast_ != 1.0f)
        {
            r = (r - 0.5f) * contrast_ + 0.5f;
            g = (g - 0.5f) * contrast_ + 0.5f;
            b = (b - 0.5f) * contrast_ + 0.5f;
        }

        // 饱和度调整
        if (saturation_ != 1.0f)
        {
            // 计算亮度（luminance）
            float luminance = 0.299f * r + 0.587f * g + 0.114f * b;

            // 应用饱和度
            r = luminance + (r - luminance) * saturation_;
            g = luminance + (g - luminance) * saturation_;
            b = luminance + (b - luminance) * saturation_;
        }

        // 限制在0-1范围内
        r = (r < 0.0f) ? 0.0f : ((r > 1.0f) ? 1.0f : r);
        g = (g < 0.0f) ? 0.0f : ((g > 1.0f) ? 1.0f : g);
        b = (b < 0.0f) ? 0.0f : ((b > 1.0f) ? 1.0f : b);
        a = (a < 0.0f) ? 0.0f : ((a > 1.0f) ? 1.0f : a);
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

