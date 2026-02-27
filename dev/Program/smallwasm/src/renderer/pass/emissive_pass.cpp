#include "emissive_pass.h"

#include "pass_utils.h"
#include "../../demo/demo_game.h"

namespace shine { namespace renderer {

class EmissivePass final : public BasePass {
public:
    void run(DemoGame& game, const RenderPass& pass, float t, int /*ctx*/) override {
        RenderSceneAndFlush(game, pass, t);
    }
};

BasePass* GetEmissivePass() {
    static EmissivePass s_emissive;
    return &s_emissive;
}

} } // namespace shine::renderer
