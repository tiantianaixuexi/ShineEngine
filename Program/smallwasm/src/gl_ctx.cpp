#include "engine/engine.h"
#include "graphics/command_buffer.h"
#include "util/wasm_compat.h"

// ======================================================================================
// ENTRY POINT (WASM EXPORTS)
// ======================================================================================

// Access to global engine instance
#define ENGINE shine::engine::Engine::instance()


// 1. Init
extern "C"
{

	void init(int triCount) {
		ENGINE.init(triCount);
	}

	// 2. Resize
	void resize(int w, int h) {
		ENGINE.onResize(w, h);
	}

	// 3. Frame
	void frame(float t) {
		ENGINE.frame(t);
	}

	// 4. Pointer
	void pointer(float x_ndc, float y_ndc, int isDown) {
		ENGINE.pointer(x_ndc, y_ndc, isDown);
	}

	// 5. Getters for JS
	int get_context_handle() {
		return ENGINE.getCtx();
	}
	int is_inited() {
		return ENGINE.isInited() ? 1 : 0;
	}
	int get_frame_no() {
		return ENGINE.getFrameNo();
	}

	// Stats getters
	int get_cmd_count() {
		return shine::graphics::CommandBuffer::instance().getCount();
	}
	int get_draw_calls() {
		return shine::graphics::CommandBuffer::instance().getDrawCalls();
	}
	int get_vertex_count() {
		return shine::graphics::CommandBuffer::instance().getVertices();
	}
	int get_instance_count() {
		return shine::graphics::CommandBuffer::instance().getInstances();
	}


}