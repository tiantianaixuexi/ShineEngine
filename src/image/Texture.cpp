#include "Texture.h"
#include "render/resources/TextureManager.h"
#include "manager/AssetManager.h"
#include "loader/image/image_loader.h"
#include <cstring>

// 暂时引入全局上下文，后续应通过依赖注入传递
#include "../../EngineCore/engine_context.h"
extern shine::EngineContext* g_EngineContext;

namespace shine::image
{
    STexture::STexture()
    {
    }

    STexture::~STexture()
    {
        // 注意：GPU 资源应该在外部通过 ReleaseRenderResource 释放
        // 这里不自动释放，因为 TextureManager 管理生命周期updateData
    }

    void STexture::Initialize(u32 width, u32 height, const std::vector<RGBA8>& data)
    {
        _width = width;
        _height = height;
        _data = data;
    }

    void STexture::InitializeFromMemory(const unsigned char* imageData, u32 width, u32 height)
    {
        _width = width;
        _height = height;
        
        size_t pixelCount = width * height;
        _data.resize(pixelCount);
        
        if (imageData)
        {
            std::memcpy(_data.data(), imageData, pixelCount * RGBA8::size());
        }
    }

    bool STexture::InitializeFromAsset(const manager::AssetHandle& assetHandle)
    {
        if (!assetHandle.isValid() || assetHandle.type != manager::EAssetType::Image)
        {
            return false;
        }

        if (!g_EngineContext) return false;

        // 获取加载器
        auto* loader = g_EngineContext->Get<manager::AssetManager>()->GetImageLoader(assetHandle);
        if (!loader || !loader->isDecoded())
        {
            return false;
        }

        // 从加载器获取数据
        const auto& imageData = loader->getImageData();
        _width = static_cast<u32>(loader->getWidth());
        _height = static_cast<u32>(loader->getHeight());

        if (imageData.empty() || _width == 0 || _height == 0)
        {
            return false;
        }

        // 复制数据到 RGBA8 格式
        size_t pixelCount = _width * _height;
        _data.resize(pixelCount);
        std::memcpy(_data.data(), imageData.data(), imageData.size());

        return true;
    }

    shine::render::TextureHandle STexture::CreateRenderResource()
    {
        if (!isValid())
        {
            return shine::render::TextureHandle{};
        }

        // 如果已经创建过，先释放旧的
        if (_renderHandle.isValid())
        {
            ReleaseRenderResource();
        }

        if (!g_EngineContext) return shine::render::TextureHandle{};

        // 通过 TextureManager 单例创建纹理
        auto* textureManager = g_EngineContext->Get<shine::render::TextureManager>();
        if (!textureManager) return shine::render::TextureHandle{};
        
        shine::render::TextureCreateInfo createInfo;
        createInfo.width = _width;
        createInfo.height = _height;
        createInfo.data = _data.data();
        createInfo.generateMipmaps = (_minFilter == TextureFilter::LINEAR_MIPMAP_LINEAR || 
                                      _minFilter == TextureFilter::LINEAR_MIPMAP_NEAREST ||
                                      _minFilter == TextureFilter::NEAREST_MIPMAP_LINEAR ||
                                      _minFilter == TextureFilter::NEAREST_MIPMAP_NEAREST);
        createInfo.linearFilter = (_magFilter == TextureFilter::LINEAR || 
                                   _minFilter == TextureFilter::LINEAR ||
                                   _minFilter == TextureFilter::LINEAR_MIPMAP_LINEAR ||
                                   _minFilter == TextureFilter::LINEAR_MIPMAP_NEAREST);
        createInfo.clampToEdge = (_wrapS == TextureWrap::CLAMP_TO_EDGE || 
                                  _wrapT == TextureWrap::CLAMP_TO_EDGE);

        _renderHandle = textureManager->CreateTexture(createInfo);
        return _renderHandle;
    }

    void STexture::ReleaseRenderResource()
    {
        if (_renderHandle.isValid() && g_EngineContext)
        {
            auto* textureManager = g_EngineContext->Get<shine::render::TextureManager>();
            if (textureManager)
            {
                textureManager->ReleaseTexture(_renderHandle);
            }
            _renderHandle = shine::render::TextureHandle{};
        }
    }
}
