#include "post_process_pass.h"

namespace shine { namespace renderer {

class PostProcessPass final : public BasePass {
public:
    void run(DemoGame& /*game*/, const RenderPass& /*pass*/, float /*t*/, int /*ctx*/) override {}
};

BasePass* GetPostProcessPass() {
    static PostProcessPass s_post;
    return &s_post;
}

} } // namespace shine::renderer
