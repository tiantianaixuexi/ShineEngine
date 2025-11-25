#pragma once

#include "shine_define.h"
#include <memory>

namespace shine::render
{
    class RenderPipelineAsset;
    class ScriptableRenderContext;
    class RenderingData;

    /**
     * @brief 渲染管线（类似 Unity RenderPipeline）
     * 定义具体的渲染流程，在 Render() 方法中实现渲染逻辑
     */
    class RenderPipeline
    {
    public:
        explicit RenderPipeline(RenderPipelineAsset* asset);
        virtual ~RenderPipeline() = default;

        /**
         * @brief 执行渲染（类似 Unity RenderPipeline.Render）
         * @param context 渲染上下文
         * @param data 渲染数据
         */
        virtual void Render(ScriptableRenderContext& context, RenderingData& data);

        /**
         * @brief 获取渲染管线资源
         */
        RenderPipelineAsset* GetAsset() const { return m_Asset; }

    protected:
        /**
         * @brief 渲染单个相机
         */
        virtual void RenderCamera(ScriptableRenderContext& context, RenderingData& data, shine::gameplay::Camera* camera);

        /**
         * @brief 渲染不透明对象
         */
        virtual void RenderOpaqueObjects(ScriptableRenderContext& context, RenderingData& data, shine::gameplay::Camera* camera);

        /**
         * @brief 渲染透明对象
         */
        virtual void RenderTransparentObjects(ScriptableRenderContext& context, RenderingData& data, shine::gameplay::Camera* camera);

        /**
         * @brief 渲染天空盒
         */
        virtual void RenderSkybox(ScriptableRenderContext& context, RenderingData& data, shine::gameplay::Camera* camera);

        /**
         * @brief 后处理
         */
        virtual void PostProcess(ScriptableRenderContext& context, RenderingData& data, shine::gameplay::Camera* camera);

    private:
        RenderPipelineAsset* m_Asset;
    };
}

