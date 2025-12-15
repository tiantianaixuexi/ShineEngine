#pragma once

// Minimal UI system (wasm-friendly, no STL).
// Coordinate space: NDC (-1..1). Origin center. +Y up.

#include "../util/wasm_compat.h"

namespace shine { namespace ui {

class Element {
public:

  // Rect in NDC
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.2f;
  float h = 0.1f;

  unsigned  int  visible   :1   = 1;
  unsigned  int  isOver    :1   = 0;     // pointer is over
  unsigned  int  isPressed :1   = 0;   // pressed

  // Optional pixel layout (professional-ish, responsive):
  // If enabled, onResize() converts pixel sizes/offsets into NDC.
  int layout_enabled = 0;
  float anchor_ndc_x = 0.0f; // [-1..1]  (-1,-1)=bottom-left, (1,1)=top-right
  float anchor_ndc_y = 0.0f;
  float offset_px_x = 0.0f;  // +right in pixels
  float offset_px_y = 0.0f;  // +up in pixels
  float size_px_w = 0.0f;    // >0 enables px-sized element
  float size_px_h = 0.0f;

  // Last known viewport size (pixels). Updated on onResize().
  int view_w = 1;
  int view_h = 1;

  virtual ~Element() = default;

  virtual void init() {}

  virtual int hit(float px, float py) const {
    const float hx = w * 0.5f;
    const float hy = h * 0.5f;
    return (px >= (x - hx) && px <= (x + hx) && py >= (y - hy) && py <= (y + hy)) ? 1 : 0;
  }

  virtual void pointer(float px, float py, int isDown) {
    isOver = hit(px, py);
    if (!isDown) isPressed = 0;
  }

  // Called when viewport size changes (w/h are canvas pixel size).
  // Override in derived class (and call Element::onResize if you use layout).
  virtual void onResize(int view_w, int view_h) {
    this->view_w = (view_w < 1) ? 1 : view_w;
    this->view_h = (view_h < 1) ? 1 : view_h;

    if (!layout_enabled) return;
    if (view_w < 1) view_w = 1;
    if (view_h < 1) view_h = 1;

    x = anchor_ndc_x + (offset_px_x * 2.0f) / (float)view_w;
    y = anchor_ndc_y + (offset_px_y * 2.0f) / (float)view_h;
    if (size_px_w > 0.0f) w = (size_px_w * 2.0f) / (float)view_w;
    if (size_px_h > 0.0f) h = (size_px_h * 2.0f) / (float)view_h;
  }

  inline void setLayoutPx(float anchorXndc, float anchorYndc,
                          float offPxX, float offPxY,
                          float pxW, float pxH) {
    layout_enabled = 1;
    anchor_ndc_x = anchorXndc;
    anchor_ndc_y = anchorYndc;
    offset_px_x = offPxX;
    offset_px_y = offPxY;
    size_px_w = pxW;
    size_px_h = pxH;
  }


  inline void setPosPx(float pxX, float pxY) noexcept {
    offset_px_x = pxX;
    offset_px_y = pxY;
  }

  inline void setSizePx(float pxW, float pxH) noexcept {
    size_px_w = pxW;
    size_px_h = pxH;
  }

  virtual void render(int /*ctxId*/) {}
};

extern "C" int ui_tex_request_url(const char* url, int urlLen, int* out_texId, int* out_w, int* out_h);
extern "C" int ui_tex_request_dataurl(const char* dataurl, int dataLen, int* out_texId, int* out_w, int* out_h);
extern "C" int ui_tex_request_base64(const char* mime, int mimeLen, const char* b64, int b64Len,
                                     int* out_texId, int* out_w, int* out_h);

// Render helpers implemented by the demo (gl_ctx.cpp).
extern "C" void ui_draw_rect_col(int ctxId, float cx, float cy, float w, float h, float r, float g, float b);
extern "C" void ui_draw_rect_uv(int ctxId, float cx, float cy, float w, float h, int texId);
extern "C" void ui_draw_round_rect(int ctxId, float cx, float cy, float w, float h,
                                   float radius_px,
                                   float fill_r, float fill_g, float fill_b, float fill_a,
                                   int texId,
                                   float texTint_r, float texTint_g, float texTint_b, float texTint_a,
                                   float border_px,
                                   float border_r, float border_g, float border_b, float border_a,
                                   float shadow_off_px_x, float shadow_off_px_y,
                                   float shadow_blur_px, float shadow_spread_px,
                                   float shadow_r, float shadow_g, float shadow_b, float shadow_a);

} } // namespace shine::ui

