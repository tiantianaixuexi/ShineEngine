#pragma once

#include "shine_define.h"
#include "render/resources/texture_handle.h"
#include <vector>
#include <memory>

// 前向声明（避免循环依赖）
namespace shine::render
{
    class TextureManager;
}

namespace shine::manager
{
    struct AssetHandle;
}

namespace shine::image
{

#pragma pack(push, 1)

    struct RGBA8
    {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        u8 a = 0;

        RGBA8(u8 _r, u8 _g, u8 _b, u8 _a)  noexcept: r(_r), g(_g), b(_b), a(_a) {}

        RGBA8() : r(0), g(0), b(0), a(0) {}

        // 获取原始数据指针，方便传递给OpenGL
        const u8* data() const noexcept { return &r; }
        u8* data() noexcept { return &r; }

        // 静态方法：获取单个像素的字节大小
        static constexpr size_t size() noexcept { return 4; }
    };

#pragma pack(pop)

    // 游戏引擎常用的纹理格式
    enum class TextureFormat
    {
        // 基础格式
        R,                  // 单通道 (Red/Luminance)
        RG,                 // 双通道 (Red-Green/Luminance-Alpha)
        RGB,                // 三通道 (Red-Green-Blue)
        RGBA,               // 四通道 (Red-Green-Blue-Alpha)

        // 压缩格式 - BC (Block Compression)
        BC1_RGB,            // DXT1 - RGB (无Alpha或1位Alpha)
        BC1_RGBA,           // DXT1 - RGBA (1位Alpha)
        BC2_RGBA,           // DXT3 - RGBA (4位Alpha)
        BC3_RGBA,           // DXT5 - RGBA (插值Alpha)
        BC4_R,              // ATI1 - 单通道压缩
        BC4_R_SIGNED,       // ATI1 - 单通道压缩 (有符号)
        BC5_RG,             // ATI2 - 双通道压缩
        BC5_RG_SIGNED,      // ATI2 - 双通道压缩 (有符号)
        BC6H_RGB_UF16,      // BC6H - RGB 半精度浮点 (无符号)
        BC6H_RGB_F16,       // BC6H - RGB 半精度浮点 (有符号)
        BC7_RGBA,           // BC7 - 高质量RGBA压缩

        // 压缩格式 - ETC (Ericsson Texture Compression)
        ETC1_RGB,           // ETC1 - RGB
        ETC2_RGB,           // ETC2 - RGB
        ETC2_RGBA,          // ETC2 - RGBA (无Alpha或Punchthrough Alpha)
        ETC2_RGBA_EAC,      // ETC2 - RGBA with EAC
        EAC_R,              // EAC - 单通道
        EAC_R_SIGNED,       // EAC - 单通道 (有符号)
        EAC_RG,             // EAC - 双通道
        EAC_RG_SIGNED,      // EAC - 双通道 (有符号)

        // 鍘嬬缉鏍煎紡 - PVRTC (PowerVR Texture Compression)
        PVRTC_RGB_2BPP,     // PVRTC - RGB 每像素2位
        PVRTC_RGB_4BPP,     // PVRTC - RGB 每像素4位
        PVRTC_RGBA_2BPP,    // PVRTC - RGBA 每像素2位
        PVRTC_RGBA_4BPP,    // PVRTC - RGBA 每像素4位

        // 压缩格式 - ASTC (Adaptive Scalable Texture Compression)
        ASTC_RGBA_4x4,      // ASTC 4x4 块
        ASTC_RGBA_5x4,      // ASTC 5x4 块
        ASTC_RGBA_5x5,      // ASTC 5x5 块
        ASTC_RGBA_6x5,      // ASTC 6x5 块
        ASTC_RGBA_6x6,      // ASTC 6x6 块
        ASTC_RGBA_8x5,      // ASTC 8x5 块
        ASTC_RGBA_8x6,      // ASTC 8x6 块
        ASTC_RGBA_8x8,      // ASTC 8x8 块
        ASTC_RGBA_10x5,     // ASTC 10x5 块
        ASTC_RGBA_10x6,     // ASTC 10x6 块
        ASTC_RGBA_10x8,     // ASTC 10x8 块
        ASTC_RGBA_10x10,    // ASTC 10x10 块
        ASTC_RGBA_12x10,    // ASTC 12x10 块
        ASTC_RGBA_12x12,    // ASTC 12x12 块

        // 浮点格式
        R16F,               // 半精度浮点单通道
        RG16F,              // 半精度浮点双通道
        RGBA16F,            // 半精度浮点四通道
        R32F,               // 全精度浮点单通道
        RG32F,              // 全精度浮点双通道
        RGB32F,             // 全精度浮点三通道
        RGBA32F,            // 全精度浮点四通道

        // 整数格式
        R8UI,               // 无符号8位整数单通道
        R16UI,              // 无符号16位整数单通道
        R32UI,              // 无符号32位整数单通道
        RG8UI,              // 无符号8位整数双通道
        RG16UI,             // 无符号16位整数双通道
        RG32UI,             // 无符号32位整数双通道
        RGBA8UI,            // 无符号8位整数四通道
        RGBA16UI,           // 无符号16位整数四通道
        RGBA32UI,           // 无符号32位整数四通道

        // 深度/模板格式
        DEPTH16,            // 16位深度
        DEPTH24,            // 24位深度
        DEPTH32,            // 32位深度
        DEPTH32F,           // 32位浮点深度
        DEPTH24_STENCIL8,   // 24位深度 + 8位模板
        DEPTH32F_STENCIL8,  // 32位浮点深度 + 8位模板
    };

    // 图像文件格式
    enum class ImageFileFormat
    {
        PNG,
        JPEG,
        BMP,
        TGA,
        TIFF,
        WEBP,
        DDS,        // DirectDraw Surface (通用游戏纹理格式)
        KTX,        // Khronos Texture (Vulkan纹理格式)
        PVR,        // PowerVR Texture (iOS纹理格式)
        ASTC,       // ASTC Texture (移动端压缩格式)
        EXR,        // OpenEXR (HDR图像格式)
        HDR,        // 辐射度HDR
        ICO,        // Windows图标格式
        GIF,        // 图形交换格式
        PSD,        // Photoshop文档
    };

    // 纹理类型
    enum class TextureType
    {
        TEXTURE_2D,         // 2D纹理
        TEXTURE_3D,         // 3D纹理
        TEXTURE_CUBE,       // 立方体贴图
        TEXTURE_ARRAY,      // 纹理数组
        TEXTURE_CUBE_ARRAY, // 立方体贴图数组
    };

    // 纹理过滤模式
    enum class TextureFilter
    {
        NEAREST,                // 最近邻过滤
        LINEAR,                 // 线性过滤
        NEAREST_MIPMAP_NEAREST, // 最近邻 + 最近邻Mipmap
        LINEAR_MIPMAP_NEAREST,  // 线性 + 最近邻Mipmap
        NEAREST_MIPMAP_LINEAR,  // 最近邻 + 线性Mipmap
        LINEAR_MIPMAP_LINEAR,   // 双线性过滤
    };

    // 纹理环绕模式
    enum class TextureWrap
    {
        CLAMP_TO_EDGE,      // 限制到边缘
        CLAMP_TO_BORDER,    // 限制到边框
        REPEAT,             // 重复
        MIRRORED_REPEAT,    // 镜像重复
    };

    /**
     * @brief 纹理资源类（类似 UE5 的 UTexture）
     * 存储纹理数据和元数据，通过 TextureManager 创建 GPU 纹理资源
     */
    class STexture
    {
    public:
        STexture();
        ~STexture();

        /**
         * @brief 初始化纹理资源（从数据创建）
         * @param width 纹理宽度
         * @param height 纹理高度
         * @param data RGBA数据
         */
        void Initialize(u32 width, u32 height, const std::vector<RGBA8>& data);

        /**
         * @brief 从内存数据初始化
         * @param imageData RGBA字节数据
         * @param width 纹理宽度
         * @param height 纹理高度
         */
        void InitializeFromMemory(const unsigned char* imageData, u32 width, u32 height);

        /**
         * @brief 从 AssetHandle 初始化（从已加载的图片资源创建）
         * @param assetHandle 资源句柄
         * @return 成功返回true
         */
        bool InitializeFromAsset(const manager::AssetHandle& assetHandle);

        /**
         * @brief 创建 GPU 纹理资源（通过 TextureManager 单例）
         * @return 纹理句柄，失败返回无效句柄
         */
        shine::render::TextureHandle CreateRenderResource();

        /**
         * @brief 释放 GPU 纹理资源（通过 TextureManager 单例）
         */
        void ReleaseRenderResource();

        /**
         * @brief 更新纹理数据（如果已创建 GPU 资源，需要重新创建）
         */
        void setData(std::vector<unsigned char>& imageData) noexcept;
        void setData(std::vector<RGBA8>& rgbaData) noexcept;

        /**
         * @brief 更新纹理数据（保持GPU资源，只更新数据）
         * @param rgbaData 新的RGBA数据
         */
        void updateData(const std::vector<RGBA8>& rgbaData);

        // 获取纹理信息
        u32 getWidth() const noexcept { return _width; }
        u32 getHeight() const noexcept { return _height; }
        u32 getDepth() const noexcept { return _depth; }
        TextureFormat getFormat() const noexcept { return _format; }
        TextureType getType() const noexcept { return _type; }
        size_t getDataSize() const noexcept { return _data.size(); }

        // 获取原始数据指针
        const void* getDataPtr() const noexcept { return _data.data(); }
        void* getDataPtr() noexcept { return _data.data(); }

        // 获取数据向量引用
        const std::vector<RGBA8>& getData() const noexcept { return _data; }
        std::vector<RGBA8>& getData() noexcept { return _data; }

        // 设置纹理尺寸
        void setWidth(u32 width) noexcept { _width = width; }
        void setHeight(u32 height) noexcept { _height = height; }
        void setDepth(u32 depth) noexcept { _depth = depth; }
        void setFormat(TextureFormat format) noexcept { _format = format; }
        void setType(TextureType type) noexcept { _type = type; }

        // 设置纹理参数
        void setFilter(TextureFilter minFilter, TextureFilter magFilter);
        void setWrap(TextureWrap s, TextureWrap t, TextureWrap r = TextureWrap::REPEAT);

        /**
         * @brief 检查纹理资源是否有效（有数据）
         */
        bool isValid() const noexcept {
            return !_data.empty() && _width > 0 && _height > 0;
        }

        /**
         * @brief 检查是否已创建 GPU 纹理资源
         */
        bool hasRenderResource() const noexcept {
            return _renderHandle.isValid();
        }

        /**
         * @brief 获取渲染资源句柄
         */
        const shine::render::TextureHandle& getRenderHandle() const noexcept {
            return _renderHandle;
        }

        /**
         * @brief 获取纹理ID（OpenGL返回GLuint，其他API返回对应的句柄）
         * 直接返回创建时存储的ID，无需查询
         * @return 纹理ID，失败返回0
         */
        uint32_t getTextureId() const noexcept {
            return _textureId;
        }

    private:
        u32 _width = 0;
        u32 _height = 0;
        u32 _depth = 1;
        TextureFormat _format = TextureFormat::RGBA;
        TextureType _type = TextureType::TEXTURE_2D;
        TextureFilter _minFilter = TextureFilter::LINEAR_MIPMAP_LINEAR;
        TextureFilter _magFilter = TextureFilter::LINEAR;
        TextureWrap _wrapS = TextureWrap::REPEAT;
        TextureWrap _wrapT = TextureWrap::REPEAT;
        TextureWrap _wrapR = TextureWrap::REPEAT;
        
        std::vector<RGBA8> _data;

        // GPU 纹理资源句柄（通过 TextureManager 创建）
        shine::render::TextureHandle _renderHandle;
        
        // GPU 纹理ID（创建时存储，避免每次查询）
        uint32_t _textureId = 0;
    };

}

