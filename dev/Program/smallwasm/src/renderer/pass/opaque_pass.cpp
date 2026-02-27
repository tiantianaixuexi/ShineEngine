#include "opaque_pass.h"

#include "pass_utils.h"
#include "../../demo/demo_game.h"
#include "../../graphics/wasm_command_buffer.h"
#include "../../graphics/gl_api.h"
#include "../../util/wasm_compat.h"

namespace shine { namespace renderer {

class OpaquePass final : public BasePass {
public:
    void run(DemoGame& game, const RenderPass& pass, float t, int /*ctx*/) override {
        shine::graphics::CommandBuffer::Pass cmd_pass = shine::graphics::g_cmd_buffer.begin_pass();
        if (game.render_mode == 0) {
            game.update_vertices(t);
            if (game.tri_count > 0 && game.buf.data()) {
                cmd_pass.push(shine::graphics::CMD_BIND_BUFFER, GL_ARRAY_BUFFER, game.vbo, 0, 0, 0, 0, 0);
                cmd_pass.push(shine::graphics::CMD_BUFFER_DATA_F32, GL_ARRAY_BUFFER,
                              ptr_i32(game.buf.data()), game.tri_count * 3 * 5,
                              GL_DYNAMIC_DRAW, 0, 0, 0);
                cmd_pass.push(shine::graphics::CMD_BIND_VAO, game.vao_basic, 0, 0, 0, 0, 0, 0);
                cmd_pass.push(shine::graphics::CMD_USE_PROGRAM, game.prog, 0, 0, 0, 0, 0, 0);
                cmd_pass.push(shine::graphics::CMD_DRAW_ARRAYS, GL_TRIANGLES, 0, game.tri_count * 3, 0, 0, 0, 0);
            }
        } else {
            game.update_instances(t);
            if (game.inst_count > 0 && game.inst.data()) {
                cmd_pass.push(shine::graphics::CMD_BIND_BUFFER, GL_ARRAY_BUFFER, game.vbo_inst_data, 0, 0, 0, 0, 0);
                cmd_pass.push(shine::graphics::CMD_BUFFER_DATA_F32, GL_ARRAY_BUFFER,
                              ptr_i32(game.inst.data()), game.inst_count * 6,
                              GL_DYNAMIC_DRAW, 0, 0, 0);
                cmd_pass.push(shine::graphics::CMD_BIND_VAO, game.vao_inst, 0, 0, 0, 0, 0, 0);
                cmd_pass.push(shine::graphics::CMD_USE_PROGRAM, game.prog_inst, 0, 0, 0, 0, 0, 0);
                cmd_pass.push(shine::graphics::CMD_DRAW_ARRAYS_INSTANCED, GL_TRIANGLES, 0, 6, game.inst_count, 0, 0, 0);
            }
        }

        RenderSceneAndFlush(game, pass, t);
    }
};

BasePass* GetOpaquePass() {
    static OpaquePass s_opaque;
    return &s_opaque;
}

} } // namespace shine::renderer
