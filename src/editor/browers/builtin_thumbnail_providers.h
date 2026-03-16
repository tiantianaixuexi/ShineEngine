#pragma once

#include <string>
#include <unordered_map>
#include <filesystem>

#include "IAssetThumbnailProvider.h"
#include "ThumbnailProviderRegistry.h"
#include "render/resources/texture_handle.h"

namespace shine::editor::assets_brower
{
    // -------------------------------------------------------------------------
    /**
     * @brief 图片缩略图提供者
     *
     * 支持格式：.png  .jpg  .jpeg
     *
     * 首次访问时同步从磁盘加载像素数据并上传 GPU 纹理；后续帧直接使用
     * 缓存的纹理句柄（O(1) 查询）。加载失败的路径记入黑名单，避免每帧
     * 重复尝试。析构时自动通过 TextureManager 释放所有 GPU 纹理。
     *
     * 扩展示例（支持 .bmp）：
     * @code
     *   class BmpThumbnailProvider : public ImageThumbnailProvider {
     *   public:
     *       bool CanHandle(const std::filesystem::path& p) const override {
     *           auto ext = p.extension().string();
     *           std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
     *           return ext == ".bmp";
     *       }
     *       // DrawThumbnail / 加载逻辑可复用父类实现，或完全重写
     *   };
     * @endcode
     */
    class ImageThumbnailProvider : public IAssetThumbnailProvider
    {
    public:
        ~ImageThumbnailProvider() override;

        bool CanHandle(const std::filesystem::path& path) const override;

        bool DrawThumbnail(ImDrawList*                  drawList,
                           const std::filesystem::path& path,
                           ImVec2                       iconMin,
                           ImVec2                       iconMax,
                           bool                         isSelected) override;

    protected:
        /**
         * @brief 加载或从缓存获取路径对应的 GPU 纹理句柄
         *
         * 子类可重写此方法以支持额外格式（例如 .bmp、.tga），
         * 并在无法识别时调用 ImageThumbnailProvider::LoadOrGetTexture 作为回退。
         *
         * @param path 图片磁盘路径（绝对路径）
         * @return 有效的 TextureHandle，或空句柄（加载失败时）
         */
        virtual shine::render::TextureHandle LoadOrGetTexture(const std::filesystem::path& path);

        // 路径字符串 → GPU 纹理句柄（命中则直接复用）
        std::unordered_map<std::string, shine::render::TextureHandle> cache_;

        // 已确认加载失败的路径（避免每帧重试）
        std::unordered_map<std::string, bool> failedPaths_;
    };

    // -------------------------------------------------------------------------
    /**
     * @brief 模型缩略图提供者
     *
     * 支持格式：.obj  .gltf  .glb  .fbx  .dae
     *
     * 当前版本绘制蓝色等轴测方块图标作为占位符，直观区分模型文件。
     * 若要替换为实际离屏渲染预览，继承此类并重写 DrawThumbnail 即可。
     *
     * @code
     *   class RealModelThumbnailProvider : public ModelThumbnailProvider {
     *   public:
     *       bool DrawThumbnail(...) override {
     *           // 使用 OffscreenRenderer 渲染模型预览
     *       }
     *   };
     * @endcode
     */
    class ModelThumbnailProvider : public IAssetThumbnailProvider
    {
    public:
        bool CanHandle(const std::filesystem::path& path) const override;

        bool DrawThumbnail(ImDrawList*                  drawList,
                           const std::filesystem::path& path,
                           ImVec2                       iconMin,
                           ImVec2                       iconMax,
                           bool                         isSelected) override;
    };

    // -------------------------------------------------------------------------
    /**
     * @brief 向注册表注册所有内置提供者
     *
     * 注册顺序：ImageThumbnailProvider → ModelThumbnailProvider
     * 自定义提供者应在调用此函数后再调用 registry.Register()，
     * 以在默认提供者之后追加（更低优先级），或在之前注册（更高优先级）。
     */
    void RegisterBuiltinThumbnailProviders(ThumbnailProviderRegistry& registry);

} // namespace shine::editor::assets_brower
