#pragma once

#include "shine_define.h"
#include <memory>

namespace shine::render
{
    class RenderPipeline;

    /**
     * @brief 渲染管线资源设置
     */
    struct RenderPipelineAssetSettings
    {
        // 渲染质量设置
        bool enableShadows = true;
        bool enablePostProcessing = false;
        int shadowResolution = 1024;
        
        // 渲染特性
        bool enableOpaqueObjects = true;
        bool enableTransparentObjects = true;
        bool enableSkybox = true;
        
        // 性能设置
        int maxVisibleLights = 4;
        bool enableBatching = true;
    };

    /**
     * @brief 渲染管线资源（类似 Unity RenderPipelineAsset）
     * 存储渲染管线的配置数据，可以创建对应的 RenderPipeline 实例
     */
    class RenderPipelineAsset
    {
    public:
        RenderPipelineAsset() = default;
        virtual ~RenderPipelineAsset() = default;

        /**
         * @brief 创建渲染管线实例
         * @return 渲染管线实例的智能指针
         */
        virtual std::unique_ptr<RenderPipeline> CreatePipeline() = 0;

        /**
         * @brief 获取渲染管线设置
         */
        RenderPipelineAssetSettings& GetSettings() { return m_Settings; }
        const RenderPipelineAssetSettings& GetSettings() const { return m_Settings; }

        /**
         * @brief 设置渲染管线设置
         */
        void SetSettings(const RenderPipelineAssetSettings& settings) { m_Settings = settings; }

    protected:
        RenderPipelineAssetSettings m_Settings;
    };

    /**
     * @brief 默认渲染管线资源
     */
    class DefaultRenderPipelineAsset : public RenderPipelineAsset
    {
    public:
        DefaultRenderPipelineAsset();
        std::unique_ptr<RenderPipeline> CreatePipeline() override;
    };
}

