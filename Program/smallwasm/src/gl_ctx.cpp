#include "engine/engine.h"
#include "graphics/command_buffer.h"
#include "util/wasm_compat.h"

// ======================================================================================
// ENTRY POINT (WASM EXPORTS)
// ======================================================================================

// Access to global engine instance
#define ENGINE shine::engine::Engine::instance()

#define WASM_EXPORT extern "C" __attribute__((visibility("default"))) __attribute__((used))

// 1. Init
WASM_EXPORT void init(int triCount) {
    ENGINE.init(triCount);
}

// 2. Resize
WASM_EXPORT void resize(int w, int h) {
    ENGINE.onResize(w, h);
}

// 3. Frame
WASM_EXPORT void frame(float t) {
    ENGINE.frame(t);
}

// 4. Pointer
WASM_EXPORT void pointer(float x_ndc, float y_ndc, int isDown) {
    ENGINE.pointer(x_ndc, y_ndc, isDown);
}

// 5. Getters for JS
WASM_EXPORT int get_context_handle() {
    return ENGINE.getCtx();
}
WASM_EXPORT int is_inited() {
    return ENGINE.isInited() ? 1 : 0;
}
WASM_EXPORT int get_frame_no() {
    return ENGINE.getFrameNo();
}

// Stats getters
WASM_EXPORT int get_cmd_count() {
    return shine::graphics::CommandBuffer::instance().getCount();
}
WASM_EXPORT int get_draw_calls() {
    return shine::graphics::CommandBuffer::instance().getDrawCalls();
}
WASM_EXPORT int get_vertex_count() {
    return shine::graphics::CommandBuffer::instance().getVertices();
}
WASM_EXPORT int get_instance_count() {
    return shine::graphics::CommandBuffer::instance().getInstances();
}
