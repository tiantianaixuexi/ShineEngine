#include "transparent_pass.h"

#include "pass_utils.h"
#include "../../demo/demo_game.h"

namespace shine { namespace renderer {

class TransparentPass final : public BasePass {
public:
    void run(DemoGame& game, const RenderPass& pass, float t, int /*ctx*/) override {
        RenderSceneAndFlush(game, pass, t);
    }
};

BasePass* GetTransparentPass() {
    static TransparentPass s_transparent;
    return &s_transparent;
}

} } // namespace shine::renderer
