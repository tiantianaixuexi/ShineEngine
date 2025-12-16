#pragma once

#include "../util/tex_loader.h"

namespace shine::graphics {

	class TextureManager {
	public:
		static TextureManager& instance();

		// Async texture requests
		int request_url( const char* url, int urlLen, int* out_texId, int* out_w, int* out_h);
		int request_dataurl(const char* data, int dataLen, int* out_texId, int* out_w, int* out_h);
		int request_base64(const char* mime, int mimeLen, const char* b64, int b64Len,int* out_texId, int* out_w, int* out_h);


		// Callbacks from JS
		void on_loaded(int reqId, int texId, int w, int h);
		void on_failed(int reqId);

	private:
		wasm::TexLoader m_loader;
	};

}
	
#define TMINST (shine::graphics::TextureManager::instance())

extern "C" {

	// Render helpers implemented by the demo (gl_ctx.cpp).
	void ui_draw_rect_col(int ctxId, float cx, float cy, float w, float h, float r, float g, float b);
	void ui_draw_rect_uv(int ctxId, float cx, float cy, float w, float h, int texId);

}