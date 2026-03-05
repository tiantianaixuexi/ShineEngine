#include "render/resources/TextureManager.h"
#include "render/backend/render_backend.h"
#include "image/Texture.h"
#include "fmt/format.h"

namespace shine::render
{
    void TextureManager::Initialize(render::backend::IRenderBackend* renderBackend)
    {
        renderBackend_ = renderBackend;
    }

    void TextureManager::Shutdown(EngineContext& ctx)
    {
        ReleaseAllTextures();
    }

    TextureHandle TextureManager::CreateTexture(const TextureCreateInfo& info)
    {
        if (!renderBackend_)
        {
            fmt::println("TextureManager: 渲染后端未初始化");
            return TextureHandle{};
        }

        if (info.width <= 0 || info.height <= 0)
        {
            fmt::println("TextureManager: 无效的纹理尺寸");
            return TextureHandle{};
        }

        // 通过渲染后端创建纹理
        uint32_t textureId = renderBackend_->CreateTexture2D(
            info.width,
            info.height,
            info.data,
            info.generateMipmaps,
            info.linearFilter,
            info.clampToEdge
        );

        if (textureId == 0)
        {
            fmt::println("TextureManager: 创建纹理失败");
            return TextureHandle{};
        }

        // 创建句柄
        TextureHandle handle;
        handle.id = nextHandleId_++;

        // 存储纹理数据
        TextureData data;
        data.width = info.width;
        data.height = info.height;
        data.textureId = textureId;
        data.streamable = info.generateMipmaps;
        textures_[handle.id] = data;

        return handle;
    }

    void TextureManager::ReleaseTexture(const TextureHandle& handle)
    {
        if (!handle.isValid())
        {
            return;
        }

        auto it = textures_.find(handle.id);
        if (it != textures_.end())
        {
            std::string oldLabel;
#if defined(BUILD_EDITOR) && BUILD_EDITOR
            oldLabel = it->second.editorDebugLabel;
#elif defined(BUILD_RUNTIME) && BUILD_RUNTIME
            oldLabel = it->second.runtimeDebugLabel;
#endif
            if (!oldLabel.empty())
            {
                labelToTexture_.erase(oldLabel);
            }
            if (renderBackend_)
            {
                renderBackend_->ReleaseTexture(it->second.textureId);
            }
            textures_.erase(it);
        }
    }

    void TextureManager::ReleaseAllTextures()
    {
        if (renderBackend_)
        {
            for (const auto& pair : textures_)
            {
                renderBackend_->ReleaseTexture(pair.second.textureId);
            }
        }
        textures_.clear();
        labelToTexture_.clear();
    }

    uint32_t TextureManager::GetTextureId(const TextureHandle& handle) const
    {
        if (!handle.isValid())
        {
            return 0;
        }

        auto it = textures_.find(handle.id);
        if (it != textures_.end())
        {
            return it->second.textureId;
        }

        return 0;
    }

    void TextureManager::UpdateTexture(const TextureHandle& handle, const void* data, int width, int height)
    {
        if (!handle.isValid() || !renderBackend_)
        {
            return;
        }

        // 更新纹理数据
        renderBackend_->UpdateTexture2D(handle.id, width, height, data);
    }

    bool TextureManager::GetTextureSize(const TextureHandle& handle, int& width, int& height) const
    {
        if (!handle.isValid())
        {
            return false;
        }

        auto it = textures_.find(handle.id);
        if (it != textures_.end())
        {
            width = it->second.width;
            height = it->second.height;
            return true;
        }

        return false;
    }

    TextureHandle TextureManager::CreateTextureFromImage(image::STexture& texture)
    {
        if (!texture.isValid())
        {
            return TextureHandle{};
        }

        // 创建纹理
        TextureCreateInfo info;
        info.width = static_cast<int>(texture.getWidth());
        info.height = static_cast<int>(texture.getHeight());
        info.data = texture.getData().data();
        info.generateMipmaps = false; // 可以从 texture 获取设置
        info.linearFilter = true;     // 可以从 texture 获取设置
        info.clampToEdge = true;      // 可以从 texture 获取设置

        TextureHandle handle = CreateTexture(info);
        
        if (handle.isValid())
        {
            fmt::println("TextureManager: 从 STexture 创建纹理成功 - {}x{}", 
                texture.getWidth(), texture.getHeight());
        }
        else
        {
            fmt::println("TextureManager: 从 STexture 创建纹理失败");
        }

        return handle;
    }

    TextureHandle TextureManager::CreateTextureFromSTexture(image::STexture& texture)
    {
        return CreateTextureFromImage(texture);
    }

    void TextureManager::GetTextureStats(size_t& count, size_t& totalMemory) const
    {
        count = textures_.size();
        totalMemory = 0;

        for (const auto& pair : textures_)
        {
            const auto& textureData = pair.second;
            // 估算内存使用：RGBA8 格式，每像素4字节，加上可能的mipmap（估算为1.33倍）
            size_t pixelCount = static_cast<size_t>(textureData.width) * static_cast<size_t>(textureData.height);
            totalMemory += pixelCount * 4 * 4 / 3; // RGBA8 = 4字节/像素，mipmap估算为1.33倍
        }
    }

    void TextureManager::BindDebugLabel(const TextureHandle& handle, const std::string& label)
    {
        if (!handle.isValid() || label.empty())
        {
            return;
        }
        auto it = textures_.find(handle.id);
        if (it == textures_.end())
        {
            return;
        }
        std::string oldLabel;
#if defined(BUILD_EDITOR) && BUILD_EDITOR
        oldLabel = it->second.editorDebugLabel;
        it->second.editorDebugLabel = label;
#endif
#if defined(BUILD_RUNTIME) && BUILD_RUNTIME
        oldLabel = it->second.runtimeDebugLabel;
        it->second.runtimeDebugLabel = label;
#endif
        if (!oldLabel.empty())
        {
            labelToTexture_.erase(oldLabel);
        }
        labelToTexture_[label] = handle.id;
    }

    TextureHandle TextureManager::FindTextureByDebugLabel(const std::string& label) const
    {
        if (label.empty())
        {
            return TextureHandle{};
        }
        auto it = labelToTexture_.find(label);
        if (it == labelToTexture_.end())
        {
            return TextureHandle{};
        }
        return TextureHandle{ .id = it->second };
    }
}
