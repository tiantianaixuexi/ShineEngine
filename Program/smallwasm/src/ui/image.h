#pragma once

#include "ui.h"
#include "../graphics/texture_manager.h"

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
  inline void setSourceUrl(const char* url) {
      TMINST.request_url(url, wasm::raw_strlen(url), &texId, &texW, &texH);
  }

  inline void setSourceDataUrl(const char* dataUrl) {
    TMINST.request_dataurl(dataUrl, wasm::raw_strlen(dataUrl), &texId, &texW, &texH);
  }

  inline void setSourceBase64(const char* mime,  const char* b64, int b64Len) {
    TMINST.request_base64(mime, wasm::raw_strlen(mime), b64, b64Len, &texId, &texW, &texH);
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

