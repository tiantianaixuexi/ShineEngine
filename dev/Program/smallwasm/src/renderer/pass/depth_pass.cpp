#include "depth_pass.h"

#include "pass_utils.h"
#include "../../demo/demo_game.h"

namespace shine { namespace renderer {

class DepthPass final : public BasePass {
public:
    void run(DemoGame& game, const RenderPass& pass, float t, int /*ctx*/) override {
        RenderSceneAndFlush(game, pass, t);
    }
};

BasePass* GetDepthPass() {
    static DepthPass s_depth;
    return &s_depth;
}

} } // namespace shine::renderer
