#include "engine/engine.h"
#include "graphics/wasm_command_buffer.h"


// ======================================================================================
// ENTRY POINT (WASM EXPORTS)
// ======================================================================================



// 1. Init
extern "C"
{

	void init(int triCount) {
		SHINE_ENGINE.init(triCount);
	}

	// 2. Resize
	void resize(int w, int h) {
		SHINE_ENGINE.onResize(w, h);
	}

	// 3. Frame
	void frame(float t) {
		SHINE_ENGINE.frame(t);
	}

	// 4. Pointer
	void pointer(float x_ndc, float y_ndc, int isDown) {
		SHINE_ENGINE.pointer(x_ndc, y_ndc, isDown);
	}

#if defined(DEBUG) && DEBUG
	// 5. Getters for JS
	int get_context_handle() {
		return SHINE_ENGINE.getCtx();
	}
	int is_inited() {
		return SHINE_ENGINE.isInited() ? 1 : 0;
	}
	int get_frame_no() {
		return SHINE_ENGINE.getFrameNo();
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
#endif


}
