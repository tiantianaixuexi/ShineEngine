#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>
#include "render/resources/texture_handle.h"
#include "Engine/Macro/RuntimeEditorSplit.h"
#include "EngineCore/subsystem.h"
#include "EngineCore/engine_context.h"

// 前向声明
namespace shine::image
{
    class STexture;
}

namespace shine::render::backend
{
    class IRenderBackend;
}

namespace shine::render
{

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
    class TextureManager : public shine::Subsystem
    {
    public:
        static TextureManager& get() { return *shine::EngineContext::Get().GetSystem<TextureManager>(); }

    public:
        /**
         * @brief 初始化纹理管理器（必须在首次使用前调用）
         * @param renderBackend 渲染后端指针
         */
        void Initialize(render::backend::IRenderBackend* renderBackend);
        virtual void Shutdown(EngineContext& ctx) override;

        /**
         * @brief 从RGBA数据创建纹理
         * @param info 纹理创建信息
         * @return 纹理句柄，失败返回无效句柄
         */
        TextureHandle CreateTexture(const TextureCreateInfo& info);
        TextureHandle CreateTextureFromImage(image::STexture& texture);

        /**
         * @brief 更新纹理数据
         * @param handle 纹理句柄
         * @param data 新的纹理数据（RGBA格式）
         * @param width 纹理宽度
         * @param height 纹理高度
         */
        void UpdateTexture(const TextureHandle& handle, const void* data, int width, int height);

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

        void BindDebugLabel(const TextureHandle& handle, const std::string& label);
        TextureHandle FindTextureByDebugLabel(const std::string& label) const;

    private:
        /**
         * @brief 内部纹理数据
         */
        struct TextureData
        {
            RUNTIME_DATA(uint32_t textureId = 0;)
            RUNTIME_DATA(bool streamable = false;)
            int width = 0;
            int height = 0;
            EDITOR_DATA(std::string importSettingsProfile;)
            EDITOR_DATA(uint64_t thumbnailCacheKey = 0;)
            EDITOR_DATA(std::string editorDebugLabel;)
            RUNTIME_DATA(std::string runtimeDebugLabel;)
        };

        render::backend::IRenderBackend* renderBackend_ = nullptr;
        std::unordered_map<uint64_t, TextureData> textures_;
        std::unordered_map<std::string, uint64_t> labelToTexture_;
        uint64_t nextHandleId_ = 1;

    private:
    };
}
