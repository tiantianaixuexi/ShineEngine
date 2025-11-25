#include "Texture.h"
#include "render/resources/texture_manager.h"
#include "manager/AssetManager.h"
#include "loader/image/image_loader.h"
#include <cstring>

namespace shine::image
{
    STexture::STexture()
    {
    }

    STexture::~STexture()
    {
        // 注意：GPU 资源应该在外部通过 ReleaseRenderResource 释放
        // 这里不自动释放，因为 TextureManager 管理生命周期
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

        // 获取加载器
        auto* loader = manager::AssetManager::Get().GetImageLoader(assetHandle);
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

        // 通过 TextureManager 单例创建纹理
        auto& textureManager = shine::render::TextureManager::get();
        
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
                                  _wrapS == TextureWrap::CLAMP_TO_BORDER);

        _renderHandle = textureManager.CreateTexture(createInfo);
        
        // 创建后立即获取并存储纹理ID
        if (_renderHandle.isValid())
        {
            _textureId = textureManager.GetTextureId(_renderHandle);
        }
        else
        {
            _textureId = 0;
        }
        
        return _renderHandle;
    }

    void STexture::ReleaseRenderResource()
    {
        if (_renderHandle.isValid())
        {
            auto& textureManager = shine::render::TextureManager::get();
            textureManager.ReleaseTexture(_renderHandle);
            _renderHandle = shine::render::TextureHandle{};
            _textureId = 0;  // 清除纹理ID
        }
    }

    void STexture::setData(std::vector<unsigned char>& imageData) noexcept
    {
        // 假设输入数据是RGBA格式，每个像素4字节
        size_t pixelCount = imageData.size() / RGBA8::size();
        _data.resize(pixelCount);

        // 直接复制字节数据到RGBA8结构体数组
        std::memcpy(_data.data(), imageData.data(), imageData.size());
    }

    void STexture::setData(std::vector<RGBA8>& rgbaData) noexcept
    {
        _data = std::move(rgbaData);
    }

    void STexture::updateData(const std::vector<RGBA8>& rgbaData)
    {
        if (rgbaData.size() != _data.size())
        {
            // 数据大小不匹配，重新设置数据
            _data = rgbaData;
            if (_renderHandle.isValid())
            {
                ReleaseRenderResource();
                CreateRenderResource();
            }
            return;
        }

        // 数据大小相同，更新现有数据和GPU纹理
        _data = rgbaData;

        // 如果有GPU资源，更新它
        if (_renderHandle.isValid())
        {
            auto& textureManager = shine::render::TextureManager::get();
            textureManager.UpdateTexture(_renderHandle, rgbaData.data(), _width, _height);
        }
    }

    // 获取纹理信息（已在头文件中内联实现，这里不需要重复）

    // 设置纹理参数
    void STexture::setFilter(TextureFilter minFilter, TextureFilter magFilter)
    {
        _minFilter = minFilter;
        _magFilter = magFilter;
    }

    void STexture::setWrap(TextureWrap s, TextureWrap t, TextureWrap r)
    {
        _wrapS = s;
        _wrapT = t;
        _wrapR = r;
    }


}


