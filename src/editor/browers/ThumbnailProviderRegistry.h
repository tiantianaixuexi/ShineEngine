#pragma once

#include <memory>
#include <vector>
#include <filesystem>

#include "IAssetThumbnailProvider.h"

namespace shine::editor::assets_brower
{
    /**
     * @brief 缩略图提供者注册表
     *
     * 维护一个有序提供者列表；Find() 返回第一个 CanHandle(path) 为 true 的提供者。
     * 先 Register() 的提供者拥有更高优先级。
     *
     * 典型用法：
     * @code
     *   ThumbnailProviderRegistry registry;
     *   registry.Register(std::make_unique<ImageThumbnailProvider>());
     *   registry.Register(std::make_unique<ModelThumbnailProvider>());
     *   registry.Register(std::make_unique<MyCustomProvider>());
     *
     *   // 每帧渲染时
     *   registry.TickAll();
     *   if (auto* p = registry.Find(path))
     *       p->DrawThumbnail(drawList, path, iconMin, iconMax, isSelected);
     * @endcode
     */
    class ThumbnailProviderRegistry
    {
    public:
        /** 注册提供者（按 Register 调用顺序决定优先级，先注册优先匹配） */
        void Register(std::unique_ptr<IAssetThumbnailProvider> provider)
        {
            providers_.push_back(std::move(provider));
        }

        /**
         * @brief 查找第一个能处理 path 的提供者
         * @return 匹配的提供者指针，无匹配返回 nullptr
         */
        IAssetThumbnailProvider* Find(const std::filesystem::path& path) const
        {
            for (const auto& p : providers_)
                if (p->CanHandle(path))
                    return p.get();
            return nullptr;
        }

        /** 调用所有已注册提供者的 Tick() */
        void TickAll()
        {
            for (auto& p : providers_)
                p->Tick();
        }

        /** 销毁所有提供者（释放 GPU 资源），应在引擎上下文销毁前调用 */
        void Clear()
        {
            providers_.clear();
        }

    private:
        std::vector<std::unique_ptr<IAssetThumbnailProvider>> providers_;
    };

} // namespace shine::editor::assets_brower
