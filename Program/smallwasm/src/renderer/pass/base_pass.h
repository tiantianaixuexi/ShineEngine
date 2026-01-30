#pragma once

#include "../render_pipeline.h"

class DemoGame;

namespace shine { namespace renderer {

class BasePass {
public:
    virtual ~BasePass() = default;
    virtual void run(DemoGame& game, const RenderPass& pass, float t, int ctx) = 0;
};

BasePass* GetUiPass();
BasePass* GetPostProcessPass();
BasePass* GetOpaquePass();
BasePass* GetDepthPass();
BasePass* GetTransparentPass();
BasePass* GetEmissivePass();

void RunDemoPasses(DemoGame& game,
                   const RenderPass* passes,
                   unsigned int pass_count,
                   float t,
                   int ctx);

} } // namespace shine::renderer
