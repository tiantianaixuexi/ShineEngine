#pragma once
// ============================================================
//  IconCache — 多搜索路径、免后缀图标缓存（Subsystem）
//
//  注册：ctx.Register(new shine::editor::IconCache());
//  获取：ctx.GetSystem<shine::editor::IconCache>()->Get("mesh/cube");
// ============================================================
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "imgui/imgui.h"
#include "EngineCore/subsystem.h"
#include "EngineCore/engine_context.h"
#include "render/resources/TextureManager.h"
#include "render/resources/texture_handle.h"
#include "util/image_util.h"
#include "util/EngineDirectoryService.h"

namespace shine::editor {

class IconCache : public shine::Subsystem {
public:
    bool Init(EngineContext& ctx) override {
        auto* dirService = ctx.GetSystem<util::EngineDirectoryService>();
        if (dirService && !dirService->GetProjectRootDirectory().empty())
            searchPaths_.push_back(dirService->GetProjectRootDirectory() / "icon");
        return true;
    }

    void Shutdown(EngineContext& /*ctx*/) override {
        ids_.clear();
        handles_.clear();
        failed_.clear();
        searchPaths_.clear();
    }

    /// 添加额外搜索路径（先添加的优先级更高）
    void AddSearchPath(std::filesystem::path root) {
        searchPaths_.push_back(std::move(root));
    }

    /// 按相对路径获取图标（无需扩展名）。
    /// 例如 Get("mesh/cube") 会在每个搜索路径下查找
    /// mesh/cube.png, mesh/cube.jpg, mesh/cube.jpeg, mesh/cube.bmp,
    /// mesh/cube.tga, mesh/cube.webp，返回第一个成功加载的纹理。
    /// 也可带扩展名调用，如 Get("icon_win_open.png")。
    ImTextureID Get(std::string_view key) {
        const std::string keyStr(key);

        // 快速路径：已缓存
        auto it = ids_.find(keyStr);
        if (it != ids_.end())
            return it->second;

        // 已确认失败，不再重试
        if (failed_.count(keyStr))
            return {};

        // 先检查纹理系统是否就绪（廉价），再做磁盘 I/O
        if (!shine::EngineContext::IsInitialized())
            return {};

        auto* tm = shine::EngineContext::Get().GetSystem<shine::render::TextureManager>();
        if (!tm)
            return {};

        // 尝试解析到磁盘文件
        const auto resolved = Resolve(keyStr);
        if (resolved.empty()) {
            failed_.insert(keyStr);
            return {};
        }

        // 加载像素数据
        const auto imgResult = shine::util::load_image(resolved.string(), 4);
        if (!imgResult) {
            failed_.insert(keyStr);
            return {};
        }

        const auto& img = *imgResult;
        if (img.width <= 0 || img.height <= 0 || img.data.empty()) {
            failed_.insert(keyStr);
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
        if (!handle.isValid()) {
            failed_.insert(keyStr);
            return {};
        }

        const uint32_t    glId = tm->GetTextureId(handle);
        const ImTextureID tid  = (ImTextureID)(uintptr_t)glId;

        handles_[keyStr] = handle;
        ids_[keyStr]     = tid;
        return tid;
    }

private:
    /// 在搜索路径中查找文件，支持带扩展名和不带扩展名两种调用方式
    std::filesystem::path Resolve(const std::string& key) const {
        static constexpr const char* kExtensions[] = {
            ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".webp"
        };

        const std::filesystem::path rel(key);
        const bool hasExt = rel.has_extension();

        for (const auto& root : searchPaths_) {
            if (hasExt) {
                // 带扩展名：直接查找
                auto full = root / rel;
                if (std::filesystem::exists(full))
                    return full;
            } else {
                // 无扩展名：逐个尝试已知图片后缀
                for (const char* ext : kExtensions) {
                    auto full = root / rel;
                    full.replace_extension(ext);
                    if (std::filesystem::exists(full))
                        return full;
                }
            }
        }
        return {};
    }

private:
    std::vector<std::filesystem::path>                        searchPaths_;
    std::unordered_map<std::string, ImTextureID>              ids_;
    std::unordered_map<std::string, shine::render::TextureHandle> handles_;
    std::unordered_set<std::string>                           failed_;
};

} // namespace shine::editor
