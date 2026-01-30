#include "pass_utils.h"

#include "../../demo/demo_game.h"
#include "../../graphics/wasm_command_buffer.h"
#include "../renderer_3d.h"

namespace shine { namespace renderer {

void RenderSceneAndFlush(DemoGame& game, const RenderPass& pass, float t) {
    game.rc.pass = pass.type;
    game.rc.sortKey = pass.sortKey;
    game.scene.render(game.rc, t);

    Renderer3D& r3d = Renderer3D::instance();
    if (pass.type == PASS_DEPTH) {
        if (r3d.depth_count() == 0u) return;
    } else if (pass.type == PASS_OPAQUE) {
        if (r3d.opaque_count() == 0u) return;
    } else if (pass.type == PASS_TRANSPARENT) {
        if (r3d.transparent_count() == 0u) return;
    } else if (pass.type == PASS_EMISSIVE) {
        if (r3d.emissive_count() == 0u) return;
    }

    shine::graphics::CommandBuffer::Pass cmd_pass = shine::graphics::g_cmd_buffer.begin_pass();
    if (pass.type == PASS_DEPTH) {
        r3d.flush_depth(cmd_pass);
    } else if (pass.type == PASS_OPAQUE) {
        r3d.flush_opaque(cmd_pass);
    } else if (pass.type == PASS_TRANSPARENT) {
        r3d.flush_transparent(cmd_pass);
    } else if (pass.type == PASS_EMISSIVE) {
        r3d.flush_emissive(cmd_pass);
    }
}

} } // namespace shine::renderer
