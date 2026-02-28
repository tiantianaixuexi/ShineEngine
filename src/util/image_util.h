#pragma once

#include "GL/glew.h"
#include "imgui/imgui.h"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace shine::util
{

// ============================================================================
// 图像数据结构
// ============================================================================

/**
 * @brief 图像数据结构
 */
struct ImageData
{
    std::vector<std::byte> data;     // 像素数据
    std::int32_t width = 0;               // 图像宽度
    std::int32_t height = 0;              // 图像高度
    std::int32_t channels = 0;            // 通道数 (1=灰度, 3=RGB, 4=RGBA)
    GLenum format = GL_RGBA;         // OpenGL 格式
    GLenum internalFormat = GL_RGBA8;// OpenGL 内部格式

    [[nodiscard]] bool valid() const noexcept { return !data.empty() && width > 0 && height > 0; }
    [[nodiscard]] size_t size() const noexcept { return data.size(); }
    [[nodiscard]] const std::byte* raw() const noexcept { return data.data(); }
};

/**
 * @brief 纹理 ID 包装器 (RAII)
 */
struct GlTexture
{
    GLuint id = 0;

    GlTexture() = default;
    explicit GlTexture(GLuint textureId) : id(textureId) {}

    // 禁用复制
    GlTexture(const GlTexture&) = delete;
    GlTexture& operator=(const GlTexture&) = delete;

    // 移动构造
    GlTexture(GlTexture&& other) noexcept : id(other.id) { other.id = 0; }
    GlTexture& operator=(GlTexture&& other) noexcept
    {
        if (this != &other)
        {
            release();
            id = other.id;
            other.id = 0;
        }
        return *this;
    }

    ~GlTexture() { release(); }

    void release()
    {
        if (id != 0)
        {
            glDeleteTextures(1, &id);
            id = 0;
        }
    }

    [[nodiscard]] bool valid() const noexcept { return id != 0; }
    [[nodiscard]] GLuint get() const noexcept { return id; }
};

/**
 * @brief 图像文件格式
 */
enum class ImageFormat : uint8_t
{
    Unknown = 0,
    PNG,
    JPEG,
    BMP,
    TGA,
    WEBP
};

/**
 * @brief 完整图像资源 (CPU数据 + GPU纹理 + 元数据)
 */
struct Image
{
    // CPU 端数据
    std::vector<std::byte> pixels;      // 像素数据
    std::int32_t width = 0;             // 图像宽度
    std::int32_t height = 0;            // 图像高度
    std::int32_t channels = 0;          // 通道数 (1=灰度, 3=RGB, 4=RGBA)
    
    // GPU 端资源
    GLuint textureId = 0;               // OpenGL 纹理 ID (外部管理生命周期)
    
    // 元数据
    ImageFormat format = ImageFormat::Unknown;  // 源文件格式
    GLenum glFormat = GL_RGBA;                  // OpenGL 格式
    GLenum glInternalFormat = GL_RGBA8;         // OpenGL 内部格式
    size_t fileSize = 0;                        // 原始文件大小 (字节)

    [[nodiscard]] bool hasPixels() const noexcept { return !pixels.empty(); }
    [[nodiscard]] bool hasTexture() const noexcept { return textureId != 0; }
    [[nodiscard]] bool valid() const noexcept { return hasPixels() && width > 0 && height > 0; }
    [[nodiscard]] size_t pixelCount() const noexcept { return static_cast<size_t>(width) * height; }
    [[nodiscard]] size_t dataSize() const noexcept { return pixels.size(); }
    [[nodiscard]] const std::byte* raw() const noexcept { return pixels.data(); }
    
    // 计算内存占用
    [[nodiscard]] size_t memoryUsage() const noexcept
    {
        return pixels.size() + (hasTexture() ? static_cast<size_t>(width) * height * channels : 0);
    }
};

// ============================================================================
// 图像文件读取
// ============================================================================

/**
 * @brief 读取图像文件原始字节
 * @param path 文件路径
 * @return 成功返回字节数组，失败返回错误信息
 */
std::expected<std::vector<std::byte>, std::string> read_image_bytes(std::string_view path);

/**
 * @brief 读取图像文件 (自动识别格式: PNG/JPEG等)
 * @param path 文件路径
 * @param desiredChannels 期望的通道数 (0=保持原样, 1=灰度, 3=RGB, 4=RGBA)
 * @return 成功返回 ImageData，失败返回错误信息
 */
std::expected<ImageData, std::string> load_image(std::string_view path, int32_t desiredChannels = 4);

/**
 * @brief 从内存数据加载图像
 * @param data 图像文件数据 (如 PNG/JPEG 编码的字节)
 * @param desiredChannels 期望的通道数
 * @return 成功返回 ImageData，失败返回错误信息
 */
std::expected<ImageData, std::string> load_image_from_memory(std::span<const std::byte> data, int32_t desiredChannels = 4);

/**
 * @brief 加载完整图像资源 (包含元数据)
 * @param path 文件路径
 * @param desiredChannels 期望的通道数
 * @return 成功返回 Image，失败返回错误信息
 */
std::expected<Image*, std::string> load_image_full(std::string_view path, int32_t desiredChannels = 4);

/**
 * @brief 加载图像并创建纹理 (一站式便捷函数)
 * @param path 文件路径
 * @param desiredChannels 期望的通道数
 * @param generateMipmaps 是否生成 mipmaps
 * @return 成功返回 Image (含像素+纹理+元数据)，失败返回错误信息
 */
std::expected<Image*, std::string> load_image_with_texture(std::string_view path, int32_t desiredChannels = 4, bool generateMipmaps = false);

// ============================================================================
// OpenGL 纹理创建
// ============================================================================

/**
 * @brief 从图像数据创建 OpenGL 纹理
 * @param image 图像数据
 * @param generateMipmaps 是否生成 mipmaps
 * @return 成功返回纹理 ID，失败返回错误信息
 */
std::expected<GlTexture, std::string> create_texture(const ImageData& image, bool generateMipmaps = true);

/**
 * @brief 从文件加载图像并创建纹理 (便捷函数)
 * @param path 图像文件路径
 * @param generateMipmaps 是否生成 mipmaps
 * @return 成功返回纹理 ID，失败返回错误信息
 */
std::expected<GlTexture, std::string> load_texture(std::string_view path, bool generateMipmaps = true);

// ============================================================================
// ImGui 纹理集成
// ============================================================================

/**
 * @brief 获取纹理的 ImGui 纹理指针
 * @param texture OpenGL 纹理
 * @return ImGui 纹理指针 (用于 ImGui::Image)
 */
inline ImTextureID to_imtexture_id(const GlTexture& texture) noexcept
{
    return (ImTextureID)(static_cast<intptr_t>(texture.get()));
}

/**
 * @brief 获取 GLuint 纹理 ID 的 ImGui 纹理指针
 * @param textureId OpenGL 纹理 ID
 * @return ImGui 纹理指针
 */
inline ImTextureID to_imtexture_id(GLuint textureId) noexcept
{
    return (ImTextureID)(static_cast<intptr_t>(textureId));
}

}

