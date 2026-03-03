#pragma once



namespace shine::gameplay
{
    class Camera;
}

#include <vector>
#include <memory>
#include "render_pass.h"

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

        void AddPass(std::unique_ptr<RenderPass> pass);
        void ClearPasses();
        void SortPasses();
        const std::vector<std::unique_ptr<RenderPass>>& GetPasses() const { return m_Passes; }

    protected:
        std::vector<std::unique_ptr<RenderPass>> m_Passes;

    private:
        RenderPipelineAsset* m_Asset;
    };
}

