#include "render/resources/texture_manager.h"
#include "render/backend/render_backend.h"
#include "image/Texture.h"
#include "fmt/format.h"
#include "util/timer/function_timer.h"

namespace shine::render
{
    TextureManager::~TextureManager()
    {
        ReleaseAllTextures();
    }

    void TextureManager::Initialize(render::backend::IRenderBackend* renderBackend)
    {
        renderBackend_ = renderBackend;
    }

    TextureHandle TextureManager::CreateTextureFromAsset(const manager::AssetHandle& assetHandle)
    {
        if (!assetHandle.isValid() || assetHandle.type != manager::EAssetType::Image)
        {
            return TextureHandle{};
        }

        // 检查是否已经创建过纹理
        for (const auto& pair : textures_)
        {
            if (pair.second.assetHandle.id == assetHandle.id)
            {
                TextureHandle handle;
                handle.id = pair.first;
                return handle;
            }
        }

        // 获取加载器
        auto* loader = manager::AssetManager::Get().GetImageLoader(assetHandle);
        if (!loader || !loader->isDecoded())
        {
            fmt::println("TextureManager: 无法获取图片加载器或图片未解码");
            return TextureHandle{};
        }

        // 从加载器获取数据
        const auto& imageData = loader->getImageData();
        int width = static_cast<int>(loader->getWidth());
        int height = static_cast<int>(loader->getHeight());

        if (imageData.empty() || width <= 0 || height <= 0)
        {
            fmt::println("TextureManager: 图片数据无效");
            return TextureHandle{};
        }

        // 创建纹理
        TextureCreateInfo info;
        info.width = width;
        info.height = height;
        info.data = imageData.data();
        info.generateMipmaps = false;
        info.linearFilter = true;
        info.clampToEdge = true;

        TextureHandle handle = CreateTexture(info);
        if (handle.isValid())
        {
            textures_[handle.id].assetHandle = assetHandle;
        }

        return handle;
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

        // 保存纹理数据
        TextureData textureData;
        textureData.textureId = textureId;
        textureData.width = info.width;
        textureData.height = info.height;

        textures_[handle.id] = textureData;

        fmt::println("TextureManager: 纹理创建成功 - {}x{} - ID: {}", info.width, info.height, textureId);

        return handle;
    }

    TextureHandle TextureManager::CreateTextureFromFile(const std::string& filePath)
    {
        shine::util::FunctionTimer timer("TextureManager::CreateTextureFromFile", shine::util::TimerPrecision::Nanoseconds);

        // 使用 AssetManager 加载图片
        auto assetHandle = manager::AssetManager::Get().LoadTextureAsset(filePath);
        if (!assetHandle.isValid())
        {
            fmt::println("TextureManager: 图片加载失败: {}", filePath);
            return TextureHandle{};
        }

        // 从资源创建纹理
        return CreateTextureFromAsset(assetHandle);
    }

    TextureHandle TextureManager::CreateTextureFromMemory(const void* data, size_t size, const std::string& formatHint)
    {
        shine::util::FunctionTimer timer("TextureManager::CreateTextureFromMemory", shine::util::TimerPrecision::Nanoseconds);

        // 使用 AssetManager 从内存加载图片
        auto assetHandle = manager::AssetManager::Get().LoadImageFromMemory(data, size, formatHint);
        if (!assetHandle.isValid())
        {
            fmt::println("TextureManager: 从内存加载图片失败");
            return TextureHandle{};
        }

        // 从资源创建纹理
        return CreateTextureFromAsset(assetHandle);
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

    void TextureManager::ReleaseTexture(const TextureHandle& handle)
    {
        if (!handle.isValid())
        {
            return;
        }

        auto it = textures_.find(handle.id);
        if (it != textures_.end())
        {
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
    }

    bool TextureManager::IsTextureValid(const TextureHandle& handle) const
    {
        if (!handle.isValid())
        {
            return false;
        }

        return textures_.find(handle.id) != textures_.end();
    }

    TextureHandle TextureManager::CreateTextureFromSTexture(image::STexture& texture)
    {
        if (!texture.isValid())
        {
            fmt::println("TextureManager: STexture 资源无效");
            return TextureHandle{};
        }

        // STexture 现在直接使用单例，不需要传递 this
        auto handle = texture.CreateRenderResource();
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

    TextureHandle TextureManager::GetTextureHandleByPath(const std::string& filePath) const
    {
        // 先通过 AssetManager 获取资源句柄
        auto assetHandle = manager::AssetManager::Get().GetAssetHandleByPath(filePath);
        if (!assetHandle.isValid())
        {
            return TextureHandle{};
        }

        // 再查找对应的纹理
        return GetTextureHandleByAsset(assetHandle);
    }

    TextureHandle TextureManager::GetTextureHandleByAsset(const manager::AssetHandle& assetHandle) const
    {
        if (!assetHandle.isValid())
        {
            return TextureHandle{};
        }

        // 查找已创建的纹理
        for (const auto& pair : textures_)
        {
            if (pair.second.assetHandle.id == assetHandle.id)
            {
                TextureHandle handle;
                handle.id = pair.first;
                return handle;
            }
        }

        return TextureHandle{};
    }
}

