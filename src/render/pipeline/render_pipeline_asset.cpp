#include "render_pipeline_asset.h"
#include "render_pipeline.h"

namespace shine::render
{
    DefaultRenderPipelineAsset::DefaultRenderPipelineAsset()
    {
        // 设置默认配置
        m_Settings.enableShadows = true;
        m_Settings.enablePostProcessing = false;
        m_Settings.shadowResolution = 1024;
        m_Settings.enableOpaqueObjects = true;
        m_Settings.enableTransparentObjects = true;
        m_Settings.enableSkybox = true;
        m_Settings.maxVisibleLights = 4;
        m_Settings.enableBatching = true;
    }

    std::unique_ptr<RenderPipeline> DefaultRenderPipelineAsset::CreatePipeline()
    {
        return std::make_unique<RenderPipeline>(this);
    }
}

