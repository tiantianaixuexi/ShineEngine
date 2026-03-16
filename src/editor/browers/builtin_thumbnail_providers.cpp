#include "builtin_thumbnail_providers.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>
#include <span>
#include <string>

#include "EngineCore/engine_context.h"
#include "render/resources/TextureManager.h"
#include "util/image_util.h"

namespace shine::editor::assets_brower
{
    // =========================================================================
    // Internal helpers
    // =========================================================================

    static std::string LowerExtension(const std::filesystem::path& path)
    {
        std::string ext = path.extension().string();
        std::ranges::transform(ext, ext.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return ext;
    }

    static ImTextureID ToImTextureID(uint32_t id) noexcept
    {
        return (ImTextureID)(uintptr_t)id;
    }

    // 缩略图最大边长（像素）——超过该尺寸的图像在上传 GPU 前先 Box-filter 降采样，
    // 避免将 4K/8K 大图整张传入显存。128×128 对图标尺寸已足够清晰。
    static constexpr int32_t kMaxThumbSize = 128;

    /**
     * @brief Box-filter 降采样（RGBA 4 通道，任意缩放比）
     *
     * 对源像素做整数步长的盒式平均（Box Filter），适合从大图生成小缩略图。
     * 时间复杂度 O(dstW × dstH × kW × kH)，对缩略图尺寸完全可接受。
     *
     * @param src     源像素（RGBA 逐行存储）
     * @param srcW    源宽度
     * @param srcH    源高度
     * @param dstW    目标宽度
     * @param dstH    目标高度
     * @return        降采样后的 RGBA 像素数据
     */
    static std::vector<std::byte> BoxFilterDownsample(
        std::span<const std::byte> src,
        int32_t srcW, int32_t srcH,
        int32_t dstW, int32_t dstH)
    {
        std::vector<std::byte> dst(static_cast<size_t>(dstW * dstH * 4));

        // 每个目标像素对应的源像素块大小（浮点，保证覆盖全图）
        const float scaleX = static_cast<float>(srcW) / static_cast<float>(dstW);
        const float scaleY = static_cast<float>(srcH) / static_cast<float>(dstH);

        for (int32_t dy = 0; dy < dstH; ++dy)
        {
            // 该行对应的源 Y 范围
            const auto fdy = static_cast<float>(dy);
            const int32_t srcY0 = static_cast<int32_t>(fdy * scaleY);
            const int32_t srcY1 = std::min(static_cast<int32_t>((fdy + 1.f) * scaleY), srcH - 1);

            for (int32_t dx = 0; dx < dstW; ++dx)
            {
                // 该列对应的源 X 范围
                const auto fdx = static_cast<float>(dx);
                const int32_t srcX0 = static_cast<int32_t>(fdx * scaleX);
                const int32_t srcX1 = std::min(static_cast<int32_t>((fdx + 1.f) * scaleX), srcW - 1);

                // 累加所有覆盖的源像素
                uint32_t r = 0, g = 0, b = 0, a = 0;
                uint32_t count = 0;
                for (int32_t sy = srcY0; sy <= srcY1; ++sy)
                {
                    for (int32_t sx = srcX0; sx <= srcX1; ++sx)
                    {
                        const auto idx = static_cast<size_t>((sy * srcW + sx) * 4);
                        r += static_cast<uint8_t>(src[idx + 0]);
                        g += static_cast<uint8_t>(src[idx + 1]);
                        b += static_cast<uint8_t>(src[idx + 2]);
                        a += static_cast<uint8_t>(src[idx + 3]);
                        ++count;
                    }
                }

                const auto dstIdx = static_cast<size_t>((dy * dstW + dx) * 4);
                dst[dstIdx + 0] = static_cast<std::byte>(r / count);
                dst[dstIdx + 1] = static_cast<std::byte>(g / count);
                dst[dstIdx + 2] = static_cast<std::byte>(b / count);
                dst[dstIdx + 3] = static_cast<std::byte>(a / count);
            }
        }
        return dst;
    }

    // =========================================================================
    // ImageThumbnailProvider
    // =========================================================================

    ImageThumbnailProvider::~ImageThumbnailProvider()
    {
        if (!shine::EngineContext::IsInitialized())
            return;
        auto* tm = shine::EngineContext::Get().GetSystem<shine::render::TextureManager>();
        if (!tm)
            return;
        for (auto& [key, handle] : cache_)
            if (handle.isValid())
                tm->ReleaseTexture(handle);
    }

    bool ImageThumbnailProvider::CanHandle(const std::filesystem::path& path) const
    {
        std::error_code ec;
        if (std::filesystem::is_directory(path, ec))
            return false;
        const std::string ext = LowerExtension(path);
        return ext == ".png" || ext == ".jpg" || ext == ".jpeg";
    }

    bool ImageThumbnailProvider::DrawThumbnail(ImDrawList*                  drawList,
                                               const std::filesystem::path& path,
                                               ImVec2                       iconMin,
                                               ImVec2                       iconMax,
                                               bool                         isSelected)
    {
        const shine::render::TextureHandle handle = LoadOrGetTexture(path);
        if (!handle.isValid())
            return false;

        if (!shine::EngineContext::IsInitialized())
            return false;
        auto* tm = shine::EngineContext::Get().GetSystem<shine::render::TextureManager>();
        if (!tm)
            return false;

        const uint32_t texId = tm->GetTextureId(handle);
        if (texId == 0)
            return false;

        // Draw image with rounded corners, UV (0,0)→(1,1) — GL coordinates
        drawList->AddImageRounded(ToImTextureID(texId),
                                  iconMin, iconMax,
                                  ImVec2(0.f, 0.f), ImVec2(1.f, 1.f),
                                  IM_COL32_WHITE, 6.0f);

        // Selection highlight border drawn on top of the image
        if (isSelected)
            drawList->AddRect(iconMin, iconMax,
                              IM_COL32(255, 205, 80, 255), 6.0f, 0, 2.0f);
        return true;
    }

    shine::render::TextureHandle ImageThumbnailProvider::LoadOrGetTexture(
        const std::filesystem::path& path)
    {
        const std::string key = path.string();

        if (auto it = cache_.find(key); it != cache_.end())
            return it->second;

        if (failedPaths_.count(key))
            return {};

        if (!shine::EngineContext::IsInitialized())
        {
            failedPaths_[key] = true;
            return {};
        }

        auto* tm = shine::EngineContext::Get().GetSystem<shine::render::TextureManager>();
        if (!tm)
        {
            failedPaths_[key] = true;
            return {};
        }

        // Use util::load_image (libpng-backed) instead of the custom png/jpeg classes
        // which have an IHDR parsing bug that returns 0x0 dimensions.
        const auto result = shine::util::load_image(key, 4);
        if (!result.has_value())
        {
            failedPaths_[key] = true;
            return {};
        }

        const shine::util::ImageData& img = *result;
        if (img.width <= 0 || img.height <= 0 || img.data.empty())
        {
            failedPaths_[key] = true;
            return {};
        }

        // 若图像超过缩略图最大尺寸，先在 CPU 端 Box-filter 降采样，
        // 避免将全分辨率大图（4K/8K 等）直接上传 GPU。
        int32_t uploadW = img.width;
        int32_t uploadH = img.height;
        std::vector<std::byte> resizedData;

        if (img.width > kMaxThumbSize || img.height > kMaxThumbSize)
        {
            const float scale = static_cast<float>(kMaxThumbSize)
                                / static_cast<float>(std::max(img.width, img.height));
            uploadW = std::max(1, static_cast<int32_t>(static_cast<float>(img.width)  * scale));
            uploadH = std::max(1, static_cast<int32_t>(static_cast<float>(img.height) * scale));
            resizedData = BoxFilterDownsample(
                img.data,
                img.width, img.height, uploadW, uploadH);
        }

        shine::render::TextureCreateInfo info;
        info.width           = uploadW;
        info.height          = uploadH;
        info.data            = resizedData.empty() ? img.data.data() : resizedData.data();
        info.generateMipmaps = false;
        info.linearFilter    = true;
        info.clampToEdge     = true;

        const auto handle = tm->CreateTexture(info);
        if (!handle.isValid())
        {
            failedPaths_[key] = true;
            return {};
        }

        cache_[key] = handle;
        return handle;
    }

    // =========================================================================
    // ModelThumbnailProvider
    // =========================================================================

    bool ModelThumbnailProvider::CanHandle(const std::filesystem::path& path) const
    {
        std::error_code ec;
        if (std::filesystem::is_directory(path, ec))
            return false;
        const std::string ext = LowerExtension(path);
        return ext == ".obj"    || ext == ".gltf" || ext == ".glb"
            || ext == ".fbx"    || ext == ".dae"
            || ext == ".sasset";
    }

    bool ModelThumbnailProvider::DrawThumbnail(ImDrawList*                  drawList,
                                               const std::filesystem::path& /*path*/,
                                               ImVec2                       iconMin,
                                               ImVec2                       iconMax,
                                               bool                         isSelected)
    {
        const float W  = iconMax.x - iconMin.x;
        const float H  = iconMax.y - iconMin.y;
        const float cx = iconMin.x + W * 0.5f;
        const float cy = iconMin.y + H * 0.48f;

        // Background
        drawList->AddRectFilled(iconMin, iconMax, IM_COL32(22, 34, 52, 240), 6.0f);

        // ---- Isometric cube (7 vertices, 3 visible faces) -------------------
        // Sizing relative to icon
        const float s  = W * 0.30f;   // half-width in screen X
        const float ys = s * 0.50f;   // Y scale (gives ~30° view angle)
        const float d  = H * 0.30f;   // cube height in screen Y

        //         v0          ← top apex
        //        /  \
        //      v3    v1       ← middle row (top face corners)
        //       \   /  \
        //        v2    v4     ← v2 is the shared centre of all 3 faces
        //        |  \  /
        //       v5    ...     (not used directly — bottom uses v4,v5,v6)
        //        |
        //        ...

        const ImVec2 v0(cx,      cy - d - ys);     // top apex
        const ImVec2 v1(cx + s,  cy - ys);          // top-right
        const ImVec2 v2(cx,      cy);               // centre (shared corner)
        const ImVec2 v3(cx - s,  cy - ys);          // top-left
        const ImVec2 v4(cx + s,  cy - ys + d);      // bottom-right
        const ImVec2 v5(cx,      cy + d);            // bottom-centre
        const ImVec2 v6(cx - s,  cy - ys + d);      // bottom-left

        // Face colours
        drawList->AddQuadFilled(v0, v1, v2, v3, IM_COL32(75, 140, 225, 200)); // top
        drawList->AddQuadFilled(v1, v4, v5, v2, IM_COL32(45,  90, 170, 200)); // right
        drawList->AddQuadFilled(v3, v2, v5, v6, IM_COL32(28,  58, 110, 200)); // left

        // Wire edges
        const ImU32 edgeCol = IM_COL32(120, 190, 255, 210);
        drawList->AddQuad(v0, v1, v2, v3, edgeCol, 1.0f);  // top face outline
        drawList->AddLine(v1, v4, edgeCol, 1.0f);
        drawList->AddLine(v4, v5, edgeCol, 1.0f);
        drawList->AddLine(v5, v6, edgeCol, 1.0f);
        drawList->AddLine(v6, v3, edgeCol, 1.0f);
        drawList->AddLine(v2, v5, edgeCol, 1.0f);  // internal vertical edge

        // Border
        const ImU32 border = isSelected ? IM_COL32(255, 205, 80, 255)
                                        : IM_COL32(70, 110, 160, 180);
        drawList->AddRect(iconMin, iconMax, border, 6.0f, 0, isSelected ? 2.0f : 1.0f);

        return true;
    }

    // =========================================================================
    // Registration helper
    // =========================================================================

    void RegisterBuiltinThumbnailProviders(ThumbnailProviderRegistry& registry)
    {
        registry.Register(std::make_unique<ImageThumbnailProvider>());
        registry.Register(std::make_unique<ModelThumbnailProvider>());
    }

} // namespace shine::editor::assets_brower
