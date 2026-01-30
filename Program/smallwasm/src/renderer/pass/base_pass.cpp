#include "base_pass.h"

#include "../../util/wasm_compat.h"

#include "depth_pass.h"
#include "emissive_pass.h"
#include "opaque_pass.h"
#include "post_process_pass.h"
#include "transparent_pass.h"
#include "ui_pass.h"

namespace shine { namespace renderer {

static SHINE_CONSTINIT BasePass* g_passHandlers[PASS_COUNT] = {};

class PassRegistry {
public:
    PassRegistry() {
        g_passHandlers[PASS_UI] = GetUiPass();
        g_passHandlers[PASS_POST_PROCESS] = GetPostProcessPass();
        g_passHandlers[PASS_OPAQUE] = GetOpaquePass();
        g_passHandlers[PASS_DEPTH] = GetDepthPass();
        g_passHandlers[PASS_TRANSPARENT] = GetTransparentPass();
        g_passHandlers[PASS_EMISSIVE] = GetEmissivePass();
    }
};

static PassRegistry g_passRegistry;

void RunDemoPasses(DemoGame& game,
                   const RenderPass* passes,
                   unsigned int pass_count,
                   float t,
                   int ctx) {
    for (unsigned int i = 0; i < pass_count; ++i) {
        const RenderPass& pass = passes[i];
        BasePass* handler = g_passHandlers[pass.type];
        if (handler) handler->run(game, pass, t, ctx);
    }
}

} } // namespace shine::renderer
