#include "gl_api.h"
#include "../util/wasm_compat.h"

extern "C" int gl_create_program_from_source(int ctx, const char* vs, const char* fs) {
    int vsId = gl_create_shader(ctx, GL_VERTEX_SHADER, (int)vs, shine::wasm::raw_strlen(vs));
    int fsId = gl_create_shader(ctx, GL_FRAGMENT_SHADER, (int)fs, shine::wasm::raw_strlen(fs));
    return gl_create_program(ctx, vsId, fsId);
}
