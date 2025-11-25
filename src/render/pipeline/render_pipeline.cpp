#include "render_pipeline.h"
#include "render_pipeline_asset.h"
#include "scriptable_render_context.h"
#include "rendering_data.h"
#include "command_buffer.h"
#include "gameplay/camera.h"
#include "gameplay/object.h"
#include "gameplay/component/component.h"

namespace shine::render
{
    RenderPipeline::RenderPipeline(RenderPipelineAsset* asset)
        : m_Asset(asset)
    {
    }

    void RenderPipeline::Render(ScriptableRenderContext& context, RenderingData& data)
    {
        if (!m_Asset || !data.mainCamera)
        {
            return;
        }

        const auto& settings = m_Asset->GetSettings();

        // 渲染主相机
        RenderCamera(context, data, data.mainCamera);

        // 如果有其他相机，也可以渲染（支持多相机）
        for (auto* camera : data.cameras)
        {
            if (camera != data.mainCamera)
            {
                RenderCamera(context, data, camera);
            }
        }
    }

    void RenderPipeline::RenderCamera(ScriptableRenderContext& context, RenderingData& data, shine::gameplay::Camera* camera)
    {
        if (!camera)
        {
            return;
        }

        const auto& settings = m_Asset->GetSettings();

        // 注意：基础的 FBO 绑定、视口设置、清除等操作由后端在 RenderSceneWith 中处理
        // 这里只负责渲染逻辑，不重复设置这些基础状态

        // 渲染天空盒
        if (settings.enableSkybox)
        {
            RenderSkybox(context, data, camera);
        }

        // 渲染不透明对象
        if (settings.enableOpaqueObjects)
        {
            RenderOpaqueObjects(context, data, camera);
        }

        // 渲染透明对象
        if (settings.enableTransparentObjects)
        {
            RenderTransparentObjects(context, data, camera);
        }

        // 后处理
        if (settings.enablePostProcessing)
        {
            PostProcess(context, data, camera);
        }
    }

    void RenderPipeline::RenderOpaqueObjects(ScriptableRenderContext& context, RenderingData& data, shine::gameplay::Camera* camera)
    {
        // 创建命令缓冲区用于渲染对象
        CommandBuffer cmdBuffer;

        // 遍历场景对象，渲染不透明对象
        for (auto* obj : data.sceneObjects)
        {
            if (!obj)
            {
                continue;
            }

            // 遍历对象的组件，调用渲染回调
            for (auto& compPtr : obj->getComponents())
            {
                if (!compPtr)
                {
                    continue;
                }

                // 使用适配器将 CommandBuffer 转换为 ICommandList
                auto adapter = cmdBuffer.GetAdapter();
                compPtr->onRender(adapter);
            }
        }

        // 提交命令缓冲区
        if (cmdBuffer.GetCommandCount() > 0)
        {
            context.Submit(&cmdBuffer);
        }
    }

    void RenderPipeline::RenderTransparentObjects(ScriptableRenderContext& context, RenderingData& data, shine::gameplay::Camera* camera)
    {
        // 类似不透明对象的渲染，但需要按深度排序
        // 暂时留空，后续实现
    }

    void RenderPipeline::RenderSkybox(ScriptableRenderContext& context, RenderingData& data, shine::gameplay::Camera* camera)
    {
        // 天空盒渲染，暂时留空，后续实现
    }

    void RenderPipeline::PostProcess(ScriptableRenderContext& context, RenderingData& data, shine::gameplay::Camera* camera)
    {
        // 后处理效果，暂时留空，后续实现
    }
}

