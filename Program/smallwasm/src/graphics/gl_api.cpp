#include "gl_api.h"
#include "../util/wasm_compat.h"

extern "C" int gl_create_program_from_source(int ctx, const char* vs, const char* fs) {
    int vsId = gl_create_shader(ctx, GL_VERTEX_SHADER, (int)vs, raw_strlen(vs));
    int fsId = gl_create_shader(ctx, GL_FRAGMENT_SHADER, (int)fs, raw_strlen(fs));
    return gl_create_program(ctx, vsId, fsId);
}

int gl_create_program_from_source_ubo(int ctx, const char* vs, const char* fs, const char* blockName, int binding) {
    int prog = gl_create_program_from_source(ctx, vs, fs);
    if (prog <= 0 || !blockName) return prog;
    int idx = gl_get_uniform_block_index(ctx, prog, (int)blockName, raw_strlen(blockName));
    if (idx >= 0) {
        gl_uniform_block_binding(ctx, prog, idx, binding);
    }
    return prog;
}
