#include "Texture.h"


#include <memory>
#include <gl/glew.h>



namespace shine::image
{

    STexture::STexture()
    {
    }

    STexture::~STexture()
    { 
    }

    void STexture::InitHandle(u32 w, u32 h)
    {
        // RGBA设置为4字节对齐
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        GLuint image_texture;
        glGenTextures(1, &image_texture);
        glBindTexture(GL_TEXTURE_2D, image_texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, &_data);

        _width = w;
        _height = h;
        textureHandle = image_texture;

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

    // 获取纹理信息
    uint32_t STexture::getWidth() const noexcept
    {
        return _width;
    }

    uint32_t STexture::getHeight() const noexcept
    {
        return _height;
    }

    uint32_t STexture::getDepth() const noexcept
    {
        return _depth;
    }

    TextureFormat STexture::getFormat() const noexcept
    {
        return _format;
    }

    TextureType STexture::getType() const noexcept
    {
        return _type;
    }

    size_t STexture::getDataSize() const noexcept
    {
        return _data.size();
    }

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


