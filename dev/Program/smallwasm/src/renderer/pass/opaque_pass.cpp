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
                cmd_pass.push3(shine::graphics::CMD_BIND_BUFFER, GL_ARRAY_BUFFER, game.vbo);
                cmd_pass.push5(shine::graphics::CMD_BUFFER_DATA_F32, GL_ARRAY_BUFFER,
                              ptr_i32(game.buf.data()), game.tri_count * 3 * 5,
                              GL_DYNAMIC_DRAW);
                cmd_pass.push2(shine::graphics::CMD_BIND_VAO, game.vao_basic);
                cmd_pass.push2(shine::graphics::CMD_USE_PROGRAM, game.prog);
                cmd_pass.push4(shine::graphics::CMD_DRAW_ARRAYS, GL_TRIANGLES, 0, game.tri_count * 3);
            }
        } else {
            game.update_instances(t);
            if (game.inst_count > 0 && game.inst.data()) {
                cmd_pass.push3(shine::graphics::CMD_BIND_BUFFER, GL_ARRAY_BUFFER, game.vbo_inst_data);
                cmd_pass.push5(shine::graphics::CMD_BUFFER_DATA_F32, GL_ARRAY_BUFFER,
                              ptr_i32(game.inst.data()), game.inst_count * 6,
                              GL_DYNAMIC_DRAW);
                cmd_pass.push2(shine::graphics::CMD_BIND_VAO, game.vao_inst);
                cmd_pass.push2(shine::graphics::CMD_USE_PROGRAM, game.prog_inst);
                cmd_pass.push5(shine::graphics::CMD_DRAW_ARRAYS_INSTANCED, GL_TRIANGLES, 0, 6, game.inst_count);
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
