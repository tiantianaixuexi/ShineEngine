#pragma once

#include "ui.h"

namespace shine { namespace ui {

class Image : public Element {
public:
  // state
  int texId = 0; // JS-side texture handle (per ctx table)
  int texW = 0;
  int texH = 0;

  // Convenience: load from URL / dataURL / raw base64.
  // - Uses JS cache: if already loaded, texId is set immediately (sync).
  // - Otherwise kicks async load and texId is set on callback.
  inline void setSourceUrl(const char* url, int urlLen) {
    ui_tex_request_url(url, urlLen, &texId, &texW, &texH);
  }
  inline void setSourceUrl(const char* url) { setSourceUrl(url, shine::wasm::raw_strlen(url)); }

  inline void setSourceDataUrl(const char* dataUrl, int dataLen) {
    ui_tex_request_dataurl(dataUrl, dataLen, &texId, &texW, &texH);
  }
  inline void setSourceDataUrl(const char* dataUrl) { setSourceDataUrl(dataUrl, shine::wasm::raw_strlen(dataUrl)); }

  inline void setSourceBase64(const char* mime, int mimeLen, const char* b64, int b64Len) {
    ui_tex_request_base64(mime, mimeLen, b64, b64Len, &texId, &texW, &texH);
  }
  inline void setSourceBase64(const char* mime, const char* b64) {
    setSourceBase64(mime, shine::wasm::raw_strlen(mime), b64, shine::wasm::raw_strlen(b64));
  }

  void onResize(int view_w, int view_h) override {
    Element::onResize(view_w, view_h);
  }

  void render(int ctxId) override {
    if (texId == 0) return;
    ui_draw_rect_uv(ctxId, x, y, w, h, texId);
  }
};

} } // namespace shine::ui

