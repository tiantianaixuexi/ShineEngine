#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_set>
#include "shine_define.h"

namespace shine::render
{
    class DebugTextureSink;
    class ScriptableRenderContext;
    class RenderingData;
    class RenderPipeline;

    enum class RenderPassEvent
    {
        BeforeRendering = 0,
        BeforeShadowMap,
        AfterShadowMap,
        BeforeOpaque,
        AfterOpaque,
        BeforeSkybox,
        AfterSkybox,
        BeforeTransparent,
        AfterTransparent,
        BeforePostProcessing,
        AfterPostProcessing,
        AfterRendering
    };

    /**
     * @brief Abstract base class for all render passes.
     * Defines inputs, outputs, and execution logic.
     */
    class RenderPass
    {
    public:
        RenderPass(const std::string& name, RenderPassEvent evt)
            : m_Name(name), m_Event(evt) {}
        virtual ~RenderPass() = default;

        virtual void Configure(RenderPipeline* pipeline, RenderingData& data) {}
        virtual void Execute(ScriptableRenderContext& context, RenderingData& data) = 0;
        virtual void CollectDebugTextures(DebugTextureSink& sink) {}

        const std::string& GetName() const { return m_Name; }
        RenderPassEvent GetEvent() const { return m_Event; }

        // Dependency Graph Helpers
        void AddInput(const std::string& resourceName) { m_Inputs.insert(resourceName); }
        void AddOutput(const std::string& resourceName) { m_Outputs.insert(resourceName); }
        
        const std::unordered_set<std::string>& GetInputs() const { return m_Inputs; }
        const std::unordered_set<std::string>& GetOutputs() const { return m_Outputs; }

    protected:
        std::string m_Name;
        RenderPassEvent m_Event;
        std::unordered_set<std::string> m_Inputs;
        std::unordered_set<std::string> m_Outputs;
    };
}
