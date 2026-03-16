#pragma once

#include <filesystem>

#include "imgui/imgui.h"

namespace shine::editor::assets_brower
{
    /**
     * @brief 资产缩略图提供者抽象基类
     *
     * 继承此类并实现 CanHandle / DrawThumbnail，即可为任意资产类型
     * 定制缩略图渲染逻辑。通过 ThumbnailProviderRegistry::Register()
     * 注册后，AssetsBrower 会在渲染网格时自动调用匹配的提供者。
     *
     * 约定：
     *   - CanHandle / DrawThumbnail / Tick 均在主（渲染）线程调用，
     *     可安全使用 ImGui API。
     *   - 实现类自行管理 GPU 资源生命周期；析构时务必释放。
     *   - DrawThumbnail 返回 false 表示"未绘制"，调用方回退到默认图标。
     */
    class IAssetThumbnailProvider
    {
    public:
        virtual ~IAssetThumbnailProvider() = default;

        /**
         * @brief 判断本提供者是否处理给定路径的缩略图
         * @param path  文件或目录路径
         * @return true → 由本提供者绘制；false → 跳过
         */
        virtual bool CanHandle(const std::filesystem::path& path) const = 0;

        /**
         * @brief 在 [iconMin, iconMax] 矩形内绘制缩略图
         * @param drawList    当前窗口的 ImDrawList（主线程安全）
         * @param path        资产磁盘路径
         * @param iconMin     图标区域左上角屏幕坐标
         * @param iconMax     图标区域右下角屏幕坐标
         * @param isSelected  是否处于选中状态
         * @return true  → 已绘制，调用方不再绘制默认占位图标
         *         false → 绘制失败或资源尚未就绪，调用方绘制默认图标
         */
        virtual bool DrawThumbnail(ImDrawList*                  drawList,
                                   const std::filesystem::path& path,
                                   ImVec2                       iconMin,
                                   ImVec2                       iconMax,
                                   bool                         isSelected) = 0;

        /**
         * @brief 每帧 Tick（可选）
         *
         * 在 AssetsBrower::onRender() 开头被调用。可在此推进异步纹理
         * 上传、LRU 淘汰等操作。默认为空实现。
         */
        virtual void Tick() {}
    };

} // namespace shine::editor::assets_brower
