#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <cstdint>
#include "loader/image/image_loader.h"
#include "manager/AssetManager.h"
#include "util/singleton.h"

// 前向声明
namespace shine::image
{
    class STexture;
}

namespace shine::render
{
    /**
     * @brief 纹理句柄（跨API统一）
     */
    struct TextureHandle
    {
        uint64_t id = 0;
        bool isValid() const { return id != 0; }
    };

    /**
     * @brief 纹理创建参数
     */
    struct TextureCreateInfo
    {
        int width = 0;
        int height = 0;
        const void* data = nullptr;  // RGBA数据，每像素4字节
        bool generateMipmaps = false;
        bool linearFilter = true;    // true=LINEAR, false=NEAREST
        bool clampToEdge = true;     // true=CLAMP_TO_EDGE, false=REPEAT
    };

    /**
     * @brief 纹理管理器 - 统一管理纹理创建和生命周期
     * 支持多种图形API：OpenGL、DirectX12、WebGL2
     * 单例模式，与 AssetManager 设计一致
     */
    class TextureManager : public shine::util::Singleton<TextureManager>
    {
    public:
        /**
         * @brief 初始化纹理管理器（必须在首次使用前调用）
         * @param renderBackend 渲染后端指针
         */
        void Initialize(backend::IRenderBackend* renderBackend);


        /**
         * @brief 从AssetHandle创建纹理（从已加载的图片数据创建）
         * @param assetHandle 资源句柄
         * @return 纹理句柄，失败返回无效句柄
         */
        TextureHandle CreateTextureFromAsset(const manager::AssetHandle& assetHandle);

        /**
         * @brief 从RGBA数据创建纹理
         * @param info 纹理创建信息
         * @return 纹理句柄，失败返回无效句柄
         */
        TextureHandle CreateTexture(const TextureCreateInfo& info);

        /**
         * @brief 从文件路径创建纹理（自动加载并创建）
         * @param filePath 图片文件路径
         * @return 纹理句柄，失败返回无效句柄
         */
        TextureHandle CreateTextureFromFile(const std::string& filePath);

        /**
         * @brief 从内存数据创建纹理（自动加载并创建）
         * @param data 图片数据
         * @param size 数据大小
         * @param formatHint 格式提示（可选）
         * @return 纹理句柄，失败返回无效句柄
         */
        TextureHandle CreateTextureFromMemory(const void* data, size_t size, const std::string& formatHint = "");

        /**
         * @brief 获取纹理ID（OpenGL返回GLuint，其他API返回对应的句柄）
         * @param handle 纹理句柄
         * @return 纹理ID，失败返回0
         */
        uint32_t GetTextureId(const TextureHandle& handle) const;

        /**
         * @brief 获取纹理尺寸
         * @param handle 纹理句柄
         * @param width 输出宽度
         * @param height 输出高度
         * @return 成功返回true
         */
        bool GetTextureSize(const TextureHandle& handle, int& width, int& height) const;

        /**
         * @brief 释放纹理
         * @param handle 纹理句柄
         */
        void ReleaseTexture(const TextureHandle& handle);

        /**
         * @brief 释放所有纹理
         */
        void ReleaseAllTextures();

        /**
         * @brief 检查纹理是否存在
         * @param handle 纹理句柄
         * @return true如果纹理存在
         */
        bool IsTextureValid(const TextureHandle& handle) const;

        /**
         * @brief 从 STexture 资源创建纹理（便利方法）
         * @param texture STexture 资源对象
         * @return 纹理句柄，失败返回无效句柄
         */
        TextureHandle CreateTextureFromSTexture(image::STexture& texture);

        /**
         * @brief 获取纹理统计信息
         * @param count 输出纹理数量
         * @param totalMemory 输出总内存使用（字节，估算）
         */
        void GetTextureStats(size_t& count, size_t& totalMemory) const;

        /**
         * @brief 根据文件路径获取纹理句柄（如果已创建）
         * @param filePath 文件路径
         * @return 纹理句柄，如果不存在返回无效句柄
         */
        TextureHandle GetTextureHandleByPath(const std::string& filePath) const;

        /**
         * @brief 根据 AssetHandle 获取纹理句柄（如果已创建）
         * @param assetHandle 资源句柄
         * @return 纹理句柄，如果不存在返回无效句柄
         */
        TextureHandle GetTextureHandleByAsset(const manager::AssetHandle& assetHandle) const;

    private:
        /**
         * @brief 内部纹理数据
         */
        struct TextureData
        {
            uint32_t textureId = 0;  // API特定的纹理ID
            int width = 0;
            int height = 0;
            manager::AssetHandle assetHandle;  // 关联的资源句柄（如果有）
        };

        backend::IRenderBackend* renderBackend_ = nullptr;
        std::unordered_map<uint64_t, TextureData> textures_;
        uint64_t nextHandleId_ = 1;

    protected:
        // 单例模式：保护构造函数和析构函数
        TextureManager() = default;
        ~TextureManager();

    private:
        // 禁止拷贝和移动
        TextureManager(const TextureManager&) = delete;
        TextureManager& operator=(const TextureManager&) = delete;
        TextureManager(TextureManager&&) = delete;
        TextureManager& operator=(TextureManager&&) = delete;
    };
}

