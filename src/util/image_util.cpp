#include "image_util.h"

#include "libpng/png.h"
#include "image/jpeg.h"

#include "fmt/format.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <setjmp.h>

namespace shine::util
{

// ============================================================================
// 图像文件读取实现
// ============================================================================

std::expected<std::vector<std::byte>, std::string> read_image_bytes(std::string_view path)
{
    std::ifstream file(path.data(), std::ios::binary | std::ios::ate);
    if (!file)
    {
        return std::unexpected(fmt::format("无法打开图像文件: {}", path));
    }

    const auto size = file.tellg();
    if (size == -1)
    {
        return std::unexpected(fmt::format("无法获取文件大小: {}", path));
    }

    file.seekg(0, std::ios::beg);

    std::vector<std::byte> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
    {
        return std::unexpected(fmt::format("读取图像文件失败: {}", path));
    }

    return buffer;
}



// ============================================================================
// PNG 解码 (libpng)
// ============================================================================

static std::expected<ImageData, std::string> decode_png(std::span<const std::byte> data, int32_t desiredChannels)
{
    if (data.size() < 8 || png_sig_cmp(reinterpret_cast<png_const_bytep>(data.data()), 0, 8) != 0)
    {
        return std::unexpected("无效的 PNG 文件签名");
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png)
    {
        return std::unexpected("png_create_read_struct 失败");
    }

    png_infop info = png_create_info_struct(png);
    if (!info)
    {
        png_destroy_read_struct(&png, nullptr, nullptr);
        return std::unexpected("png_create_info_struct 失败");
    }

    if (setjmp(png_jmpbuf(png)))
    {
        png_destroy_read_struct(&png, &info, nullptr);
        return std::unexpected("PNG 解码错误");
    }

    // 创建可变的指针副本，供回调函数更新
    std::byte* pngData = const_cast<std::byte*>(data.data());
    png_set_read_fn(png, &pngData, [](png_structp png, png_bytep out, size_t size) {
        auto** ppData = reinterpret_cast<std::byte**>(png_get_io_ptr(png));
        std::memcpy(out, *ppData, size);
        *ppData += size;
    });

    png_read_info(png, info);

    int width = static_cast<int>(png_get_image_width(png, info));
    int height = static_cast<int>(png_get_image_height(png, info));
    png_byte colorType = png_get_color_type(png, info);
    png_byte bitDepth = png_get_bit_depth(png, info);

    // 转换格式
    if (bitDepth == 16)
        png_set_strip_16(png);
    if (colorType == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);
    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
        png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    // 根据期望通道数设置转换
    int outChannels = desiredChannels > 0 ? desiredChannels : 4;
    if (outChannels == 4)
    {
        if (colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_PALETTE)
            png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
        if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
            png_set_gray_to_rgb(png);
    }
    else if (outChannels == 3)
    {
        if (colorType == PNG_COLOR_TYPE_GRAY_ALPHA || colorType == PNG_COLOR_TYPE_RGBA)
            png_set_strip_alpha(png);
        if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
            png_set_gray_to_rgb(png);
    }
    else if (outChannels == 1)
    {
        png_set_rgb_to_gray(png, 1, 0.299, 0.587);
        png_set_strip_alpha(png);
    }

    png_read_update_info(png, info);

    // 重新获取转换后的通道数
    const size_t rowBytes = png_get_rowbytes(png, info);
    const int actualChannels = static_cast<int>(rowBytes / width);

    ImageData image;
    image.width = width;
    image.height = height;
    image.channels = actualChannels;
    image.data.resize(static_cast<size_t>(height) * rowBytes);

    std::vector<png_bytep> rowPointers(height);
    for (int y = 0; y < height; ++y)
    {
        rowPointers[y] = reinterpret_cast<png_bytep>(image.data.data() + static_cast<size_t>(y) * rowBytes);
    }

    png_read_image(png, rowPointers.data());
    png_read_end(png, nullptr);

    png_destroy_read_struct(&png, &info, nullptr);

    // 设置 OpenGL 格式
    switch (image.channels)
    {
        case 1:
            image.format = GL_RED;
            image.internalFormat = GL_R8;
            break;
        case 3:
            image.format = GL_RGB;
            image.internalFormat = GL_RGB8;
            break;
        case 4:
        default:
            image.format = GL_RGBA;
            image.internalFormat = GL_RGBA8;
            break;
    }

    return image;
}

// ============================================================================
// JPEG 解码 (libjpeg)
// ============================================================================

// struct JpegErrorMgr
// {
//     struct jpeg_error_mgr pub;
//     jmp_buf setjmpBuffer;
//     std::string errorMessage;
// };

// static void jpeg_error_exit(j_common_ptr cinfo)
// {
//     auto* err = reinterpret_cast<JpegErrorMgr*>(cinfo->err);
//     char buffer[JMSG_LENGTH_MAX];
//     (*cinfo->err->format_message)(cinfo, buffer);
//     err->errorMessage = buffer;
//     longjmp(err->setjmpBuffer, 1);
// }

// static std::expected<ImageData, std::string> decode_jpeg(std::span<const std::byte> data, int32_t desiredChannels)
// {
//     if (data.size() < 2 || data[0] != std::byte{0xFF} || data[1] != std::byte{0xD8})
//     {
//         return std::unexpected("无效的 JPEG 文件签名");
//     }

//     jpeg_decompress_struct cinfo;
//     JpegErrorMgr jerr;

//     cinfo.err = jpeg_std_error(&jerr.pub);
//     jerr.pub.error_exit = jpeg_error_exit;

//     if (setjmp(jerr.setjmpBuffer))
//     {
//         jpeg_destroy_decompress(&cinfo);
//         return std::unexpected(fmt::format("JPEG 解码错误: {}", jerr.errorMessage));
//     }

//     jpeg_create_decompress(&cinfo);

//     jpeg_mem_src(&cinfo, reinterpret_cast<const unsigned char*>(data.data()), data.size());

//     if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK)
//     {
//         jpeg_destroy_decompress(&cinfo);
//         return std::unexpected("无效的 JPEG 文件头");
//     }

//     int outChannels = desiredChannels > 0 ? desiredChannels : 4;
//     if (outChannels == 4 || outChannels == 3)
//     {
//         cinfo.out_color_space = JCS_RGB;
//         outChannels = 3;
//     }
//     else if (outChannels == 1)
//     {
//         cinfo.out_color_space = JCS_GRAYSCALE;
//         outChannels = 1;
//     }

//     jpeg_start_decompress(&cinfo);

//     ImageData image;
//     image.width = static_cast<int32_t>(cinfo.output_width);
//     image.height = static_cast<int32_t>(cinfo.output_height);
//     image.channels = outChannels;

//     const size_t rowStride = static_cast<size_t>(cinfo.output_width) * outChannels;
//     image.data.resize(static_cast<size_t>(image.height) * rowStride);

//     std::vector<JSAMPLE*> rowPointers(image.height);
//     for (int32_t y = 0; y < image.height; ++y)
//     {
//         rowPointers[y] = reinterpret_cast<JSAMPLE*>(image.data.data() + static_cast<size_t>(y) * rowStride);
//     }

//     while (cinfo.output_scanline < cinfo.output_height)
//     {
//         jpeg_read_scanlines(&cinfo, &rowPointers[cinfo.output_scanline], 1);
//     }

//     jpeg_finish_decompress(&cinfo);
//     jpeg_destroy_decompress(&cinfo);

//     // 如果需要 4 通道，手动添加 Alpha
//     if (desiredChannels == 4 && image.channels == 3)
//     {
//         std::vector<std::byte> rgba(static_cast<size_t>(image.width) * image.height * 4);
//         const size_t pixelCount = static_cast<size_t>(image.width) * image.height;
//         for (size_t i = 0; i < pixelCount; ++i)
//         {
//             rgba[i * 4 + 0] = image.data[i * 3 + 0];
//             rgba[i * 4 + 1] = image.data[i * 3 + 1];
//             rgba[i * 4 + 2] = image.data[i * 3 + 2];
//             rgba[i * 4 + 3] = std::byte{0xFF};
//         }
//         image.data = std::move(rgba);
//         image.channels = 4;
//     }

//     // 设置 OpenGL 格式
//     switch (image.channels)
//     {
//         case 1:
//             image.format = GL_RED;
//             image.internalFormat = GL_R8;
//             break;
//         case 3:
//             image.format = GL_RGB;
//             image.internalFormat = GL_RGB8;
//             break;
//         case 4:
//         default:
//             image.format = GL_RGBA;
//             image.internalFormat = GL_RGBA8;
//             break;
//     }

//     return image;
// }

// ============================================================================
// 通用图像加载
// ============================================================================

std::expected<ImageData, std::string> load_image_from_memory(std::span<const std::byte> data, int32_t desiredChannels)
{
    if (data.size() < 8)
    {
        return std::unexpected("数据太小，无法识别图像格式");
    }

    // 检测 PNG 签名
    if (data[0] == std::byte{0x89} && data[1] == std::byte{0x50} &&
        data[2] == std::byte{0x4E} && data[3] == std::byte{0x47})
    {
        return decode_png(data, desiredChannels);
    }

    // 检测 JPEG 签名
    if (data[0] == std::byte{0xFF} && data[1] == std::byte{0xD8})
    {
        shine::image::jpeg jpegDecoder;
        if (!jpegDecoder.loadFromMemory(data.data(), data.size()))
            return std::unexpected("JPEG: loadFromMemory 失败");

        auto decodeResult = jpegDecoder.decode();
        if (!decodeResult)
            return std::unexpected("JPEG 解码失败: " + decodeResult.error());

        const auto& rawRgba = jpegDecoder.getImageData();
        ImageData image;
        image.width    = static_cast<int32_t>(jpegDecoder.getWidth());
        image.height   = static_cast<int32_t>(jpegDecoder.getHeight());
        image.channels = 4;
        image.format   = GL_RGBA;
        image.internalFormat = GL_RGBA8;

        if (desiredChannels == 3)
        {
            // 去除 alpha 通道
            const size_t pixels = static_cast<size_t>(image.width) * image.height;
            image.data.resize(pixels * 3);
            for (size_t i = 0; i < pixels; ++i)
            {
                image.data[i * 3 + 0] = static_cast<std::byte>(rawRgba[i * 4 + 0]);
                image.data[i * 3 + 1] = static_cast<std::byte>(rawRgba[i * 4 + 1]);
                image.data[i * 3 + 2] = static_cast<std::byte>(rawRgba[i * 4 + 2]);
            }
            image.channels = 3;
            image.format   = GL_RGB;
            image.internalFormat = GL_RGB8;
        }
        else
        {
            image.data.resize(rawRgba.size());
            std::memcpy(image.data.data(), rawRgba.data(), rawRgba.size());
        }
        return image;
    }

    return std::unexpected("不支持的图像格式 (仅支持 PNG/JPEG)");
}

std::expected<ImageData, std::string> load_image(std::string_view path, int32_t desiredChannels)
{
    auto bytesResult = read_image_bytes(path);
    if (!bytesResult)
    {
        return std::unexpected(std::move(bytesResult.error()));
    }

    return load_image_from_memory(*bytesResult, desiredChannels);
}



// ============================================================================
// 完整图像加载 (含元数据)
// ============================================================================

static ImageFormat detect_image_format(std::span<const std::byte> data)
{
    if (data.size() < 8) return ImageFormat::Unknown;
    
    // PNG: 89 50 4E 47 0D 0A 1A 0A
    if (data[0] == std::byte{0x89} && data[1] == std::byte{0x50} &&
        data[2] == std::byte{0x4E} && data[3] == std::byte{0x47})
    {
        return ImageFormat::PNG;
    }
    
    // JPEG: FF D8
    if (data[0] == std::byte{0xFF} && data[1] == std::byte{0xD8})
    {
        return ImageFormat::JPEG;
    }
    
    // BMP: BM
    if (data[0] == std::byte{0x42} && data[1] == std::byte{0x4D})
    {
        return ImageFormat::BMP;
    }
    
    // TGA: 无明确签名，需检查扩展名
    // WEBP: RIFF....WEBP
    if (data.size() >= 12 && 
        data[0] == std::byte{0x52} && data[1] == std::byte{0x49} &&
        data[2] == std::byte{0x46} && data[3] == std::byte{0x46} &&
        data[8] == std::byte{0x57} && data[9] == std::byte{0x45} &&
        data[10] == std::byte{0x42} && data[11] == std::byte{0x50})
    {
        return ImageFormat::WEBP;
    }
    
    return ImageFormat::Unknown;
}

std::expected<Image*, std::string> load_image_full(std::string_view path, int32_t desiredChannels)
{
    auto bytesResult = read_image_bytes(path);
    if (!bytesResult)
    {
        return std::unexpected(std::move(bytesResult.error()));
    }

    const auto& bytes = *bytesResult;
    auto imageDataResult = load_image_from_memory(bytes, desiredChannels);
    if (!imageDataResult)
    {
        return std::unexpected(std::move(imageDataResult.error()));
    }

    const auto& imageData = *imageDataResult;
    
    Image* image = new Image();
    image->pixels = std::move(imageData.data);
    image->width = imageData.width;
    image->height = imageData.height;
    image->channels = imageData.channels;
    image->format = detect_image_format(bytes);
    image->glFormat = imageData.format;
    image->glInternalFormat = imageData.internalFormat;
    image->fileSize = bytes.size();

    return image;
}

std::expected<Image*, std::string> load_image_with_texture(std::string_view path, int32_t desiredChannels, bool generateMipmaps)
{
    auto imageResult = load_image_full(path, desiredChannels);
    if (!imageResult)
    {
        return std::unexpected(std::move(imageResult.error()));
    }

    Image* image = *imageResult;

    // 从 Image 创建纹理 (临时 ImageData 视图)
    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    if (textureId == 0)
    {
        delete image;
        return std::unexpected("创建 OpenGL 纹理失败");
    }

    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        image->glInternalFormat,
        image->width,
        image->height,
        0,
        image->glFormat,
        GL_UNSIGNED_BYTE,
        image->pixels.data());

    if (generateMipmaps)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    image->textureId = textureId;
    return image;
}


// ============================================================================
// OpenGL 纹理创建实现
// ============================================================================

std::expected<GlTexture, std::string> create_texture(const ImageData& image, bool generateMipmaps)
{
    if (!image.valid())
    {
        return std::unexpected("无效的图像数据");
    }

    GLuint textureId;
    glGenTextures(1, &textureId);
    if (textureId == 0)
    {
        return std::unexpected("创建 OpenGL 纹理失败");
    }

    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        image.internalFormat,
        image.width,
        image.height,
        0,
        image.format,
        GL_UNSIGNED_BYTE,
        image.raw());

    if (generateMipmaps)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    return GlTexture(textureId);
}

std::expected<GlTexture, std::string> load_texture(std::string_view path, bool generateMipmaps)
{
    auto imageResult = load_image(path);
    if (!imageResult)
    {
        return std::unexpected(std::move(imageResult.error()));
    }

    return create_texture(*imageResult, generateMipmaps);
}

} // namespace shine::util
