#pragma once

// tex_loader.h
// Tiny async texture request table for wasm <-> JS (callback mode with sync cache fast-path).
// - JS provides: js_tex_load_* / js_tex_load_*_sync / js_tex_get_wh
// - JS calls back: on_tex_loaded / on_tex_failed (exported from wasm)
//
// This is header-only so gl_ctx.cpp stays readable.

#include "wasm_compat.h"

extern "C" {
	void js_tex_load_url(int ctx, int url_ptr, int url_len, int reqId);
	void js_tex_load_dataurl(int ctx, int data_ptr, int data_len, int reqId);
	void js_tex_load_base64(int ctx, int mime_ptr, int mime_len, int b64_ptr, int b64_len, int reqId);
	int  js_tex_load_url_sync(int ctx, int url_ptr, int url_len);           // texId if cached else 0
	int  js_tex_load_dataurl_sync(int ctx, int data_ptr, int data_len);     // texId if cached else 0
	int  js_tex_load_base64_sync(int ctx, int mime_ptr, int mime_len, int b64_ptr, int b64_len); // texId if cached else 0
	int  js_tex_get_wh(int ctx, int texId); // packed (w<<16)|h, 0 if unknown
}

namespace shine::wasm {

	struct TexLoader {

		struct Req {
			int used = 0;
			int id = 0;
			int* out_texId = nullptr;
			int* out_w = nullptr;
			int* out_h = nullptr;
		};

		Req req[16] = {};
		int next = 1;

		inline void set_wh_from_tex(int ctxId, int texId, int* out_w, int* out_h) {
			if (!out_w && !out_h) return;
			const int wh = js_tex_get_wh(ctxId, texId);
			const int w = (wh >> 16) & 0xffff;
			const int h = (wh) & 0xffff;
			if (out_w) *out_w = w;
			if (out_h) *out_h = h;
		}

		inline int alloc_slot() {
			for (int i = 0; i < (int)(sizeof(req) / sizeof(req[0])); ++i) {
				if (!req[i].used) return i;
			}
			return -1;
		}

		inline int request_async_url(int ctxId, const char* url, int urlLen, int* out_texId, int* out_w, int* out_h) {
			if (!url || urlLen <= 0 || !out_texId) return 0;
			const int cached = js_tex_load_url_sync(ctxId, ptr_i32(url), urlLen);
			if (cached != 0) {
				*out_texId = cached;
				set_wh_from_tex(ctxId, cached, out_w, out_h);
				return 0;
			}
			const int slot = alloc_slot();
			if (slot < 0) return 0;
			const int id = next++;
			req[slot].used = 1;
			req[slot].id = id;
			req[slot].out_texId = out_texId;
			req[slot].out_w = out_w;
			req[slot].out_h = out_h;
			js_tex_load_url(ctxId, ptr_i32(url), urlLen, id);
			return id;
		}

		inline int request_async_dataurl(int ctxId, const char* data, int dataLen, int* out_texId, int* out_w, int* out_h) {
			if (!data || dataLen <= 0 || !out_texId) return 0;
			const int cached = js_tex_load_dataurl_sync(ctxId, ptr_i32(data), dataLen);
			if (cached != 0) {
				*out_texId = cached;
				set_wh_from_tex(ctxId, cached, out_w, out_h);
				return 0;
			}
			const int slot = alloc_slot();
			if (slot < 0) return 0;
			const int id = next++;
			req[slot].used = 1;
			req[slot].id = id;
			req[slot].out_texId = out_texId;
			req[slot].out_w = out_w;
			req[slot].out_h = out_h;
			js_tex_load_dataurl(ctxId, ptr_i32(data), dataLen, id);
			return id;
		}

		inline int request_async_base64(int ctxId, const char* mime, int mimeLen, const char* b64, int b64Len,
			int* out_texId, int* out_w, int* out_h) {
			if (!b64 || b64Len <= 0 || !out_texId) return 0;
			if (!mime || mimeLen <= 0) { mime = "image/png"; mimeLen = 9; }

			const int cached = js_tex_load_base64_sync(ctxId, ptr_i32(mime), mimeLen, ptr_i32(b64), b64Len);
			if (cached != 0) {
				*out_texId = cached;
				set_wh_from_tex(ctxId, cached, out_w, out_h);
				return 0;
			}
			const int slot = alloc_slot();
			if (slot < 0) return 0;
			const int id = next++;
			req[slot].used = 1;
			req[slot].id = id;
			req[slot].out_texId = out_texId;
			req[slot].out_w = out_w;
			req[slot].out_h = out_h;
			js_tex_load_base64(ctxId, ptr_i32(mime), mimeLen, ptr_i32(b64), b64Len, id);
			return id;
		}

		inline int request_sync_url(int ctxId, const char* url, int urlLen, int* out_texId, int* out_w, int* out_h) {
			if (!url || urlLen <= 0 || !out_texId) return 0;
			const int cached = js_tex_load_url_sync(ctxId, ptr_i32(url), urlLen);
			if (cached != 0) {
				*out_texId = cached;
				set_wh_from_tex(ctxId, cached, out_w, out_h);
				return 0;
			}
			const int slot = alloc_slot();
			if (slot < 0) return 0;
			const int id = next++;
			req[slot].used = 1;
			req[slot].id = id;
			req[slot].out_texId = out_texId;
			req[slot].out_w = out_w;
			req[slot].out_h = out_h;
			js_tex_load_url(ctxId, ptr_i32(url), urlLen, id);
			return id;
		}

		inline int request_sync_dataurl(int ctxId, const char* data, int dataLen, int* out_texId, int* out_w, int* out_h) {
			if (!data || dataLen <= 0 || !out_texId) return 0;
			const int cached = js_tex_load_dataurl_sync(ctxId, ptr_i32(data), dataLen);
			if (cached != 0) {
				*out_texId = cached;
				set_wh_from_tex(ctxId, cached, out_w, out_h);
				return 0;
			}
			const int slot = alloc_slot();
			if (slot < 0) return 0;
			const int id = next++;
			req[slot].used = 1;
			req[slot].id = id;
			req[slot].out_texId = out_texId;
			req[slot].out_w = out_w;
			req[slot].out_h = out_h;
			js_tex_load_dataurl(ctxId, ptr_i32(data), dataLen, id);
			return id;
		}

		inline int request_sync_base64(int ctxId, const char* mime, int mimeLen, const char* b64, int b64Len,
			int* out_texId, int* out_w, int* out_h) {
			if (!b64 || b64Len <= 0 || !out_texId) return 0;
			if (!mime || mimeLen <= 0) { mime = "image/png"; mimeLen = 9; }

			const int cached = js_tex_load_base64_sync(ctxId, ptr_i32(mime), mimeLen, ptr_i32(b64), b64Len);
			if (cached != 0) {
				*out_texId = cached;
				set_wh_from_tex(ctxId, cached, out_w, out_h);
				return 0;
			}
			const int slot = alloc_slot();
			if (slot < 0) return 0;
			const int id = next++;
			req[slot].used = 1;
			req[slot].id = id;
			req[slot].out_texId = out_texId;
			req[slot].out_w = out_w;
			req[slot].out_h = out_h;
			js_tex_load_base64(ctxId, ptr_i32(mime), mimeLen, ptr_i32(b64), b64Len, id);
			return id;
		}


		inline void on_loaded(int reqId, int texId, int w, int h) {
			for (int i = 0; i < (int)(sizeof(req) / sizeof(req[0])); ++i) {
				if (!req[i].used) continue;
				if (req[i].id != reqId) continue;
				if (req[i].out_texId) *req[i].out_texId = texId;
				if (req[i].out_w) *req[i].out_w = w;
				if (req[i].out_h) *req[i].out_h = h;
				req[i].used = 0;
				req[i].id = 0;
				req[i].out_texId = nullptr;
				req[i].out_w = nullptr;
				req[i].out_h = nullptr;
				return;
			}
		}

		inline void on_failed(int reqId) {
			for (int i = 0; i < (int)(sizeof(req) / sizeof(req[0])); ++i) {
				if (!req[i].used) continue;
				if (req[i].id != reqId) continue;
				if (req[i].out_texId) *req[i].out_texId = -1;
				req[i].used = 0;
				req[i].id = 0;
				req[i].out_texId = nullptr;
				req[i].out_w = nullptr;
				req[i].out_h = nullptr;
				return;
			}
		}
	};

}
