#include "render_pipeline.h"
#include "render_pipeline_asset.h"
#include "scriptable_render_context.h"
#include "render_data.h"
#include "command_buffer.h"
#include <algorithm>
#include <fmt/format.h>

namespace shine::render
{
    RenderPipeline::RenderPipeline(RenderPipelineAsset* asset)
        : m_Asset(asset)
    {
    }

    void RenderPipeline::AddPass(std::unique_ptr<RenderPass> pass)
    {
        if (pass)
        {
            m_Passes.push_back(std::move(pass));
        }
    }

    void RenderPipeline::SortPasses()
    {
        // 1. Sort by Event priority first
        std::stable_sort(m_Passes.begin(), m_Passes.end(), [](const auto& a, const auto& b) {
            return a->GetEvent() < b->GetEvent();
        });

        // 2. Resolve dependencies within same Event (Basic implementation)
        // If Pass B inputs contains any of Pass A outputs, ensure A comes before B.
        // Since we use stable_sort and Event order, this is mostly handled by assigning correct Events.
        // A full graph sort would require building an adjacency list.
    }

    void RenderPipeline::Render(ScriptableRenderContext& context, RenderingData& data)
    {
        if (!m_Asset || !data.mainCamera)
        {
            return;
        }

        SortPasses();

        for (auto& pass : m_Passes)
        {
            pass->Configure(this, data);
            pass->Execute(context, data);
        }
    }
}
