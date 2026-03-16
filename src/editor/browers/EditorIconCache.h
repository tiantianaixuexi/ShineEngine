#pragma once
// ============================================================
//  EditorIconCache — 按需从 icon/ 目录加载 PNG 图标，上传 GPU
//  并缓存 ImTextureID，同一文件名只加载一次（跨帧持久）。
// ============================================================
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "imgui/imgui.h"
#include "EngineCore/engine_context.h"
#include "render/resources/TextureManager.h"
#include "render/resources/texture_handle.h"
#include "util/image_util.h"

namespace shine::editor::assets_brower
{
    class EditorIconCache
    {
    public:
        void SetRoot(std::filesystem::path root) { root_ = std::move(root); }

        // 按文件名（含扩展名，如 "icon_win_close.png"）获取 ImTextureID。
        // 未找到或加载失败时返回 ImTextureID{}（即 nullptr/0）。
        ImTextureID Get(const std::string& filename)
        {
            // 快速路径：已缓存
            auto it = ids_.find(filename);
            if (it != ids_.end())
                return it->second;

            // 已确认失败，不再重试
            if (failed_.count(filename))
                return {};

            // 加载像素数据
            const auto path = root_ / filename;
            const auto imgResult = shine::util::load_image(path.string(), 4);
            if (!imgResult)
            {
                failed_.insert(filename);
                return {};
            }

            const auto& img = *imgResult;
            if (img.width <= 0 || img.height <= 0 || img.data.empty())
            {
                failed_.insert(filename);
                return {};
            }

            if (!shine::EngineContext::IsInitialized())
            {
                failed_.insert(filename);
                return {};
            }

            auto* tm = shine::EngineContext::Get().GetSystem<shine::render::TextureManager>();
            if (!tm)
            {
                failed_.insert(filename);
                return {};
            }

            // 上传 GPU
            shine::render::TextureCreateInfo info;
            info.width           = img.width;
            info.height          = img.height;
            info.data            = img.data.data();
            info.generateMipmaps = false;
            info.linearFilter    = true;
            info.clampToEdge     = true;

            const auto handle = tm->CreateTexture(info);
            if (!handle.isValid())
            {
                failed_.insert(filename);
                return {};
            }

            const uint32_t  glId = tm->GetTextureId(handle);
            const ImTextureID tid = (ImTextureID)(uintptr_t)glId;

            handles_[filename] = handle;  // 保持 GPU 生命周期
            ids_[filename]     = tid;
            return tid;
        }

    private:
        std::filesystem::path                                         root_;
        std::unordered_map<std::string, ImTextureID>                  ids_;
        std::unordered_map<std::string, shine::render::TextureHandle> handles_;
        std::unordered_set<std::string>                               failed_;
    };

} // namespace shine::editor::assets_brower
