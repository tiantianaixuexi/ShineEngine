#pragma once

// Minimal UI system (wasm-friendly, no STL).
// Coordinate space: NDC (-1..1). Origin center. +Y up.

#include "../util/wasm_compat.h"

namespace shine { namespace ui {

class Element {
public:

  // Rect in Pixels (previously NDC, now converted to Pixels for Renderer2D)
  float x = 0.0f; // Center X
  float y = 0.0f; // Center Y
  float w = 100.0f;
  float h = 50.0f;

  unsigned  int  visible   :1   = 1;
  unsigned  int  isOver    :1   = 0;     // pointer is over
  unsigned  int  isPressed :1   = 0;   // pressed

  // Optional pixel layout (professional-ish, responsive):
  // If enabled, onResize() converts layout rules into Center-Pixels.
  int layout_enabled = 0;
  float anchor_ndc_x = 0.0f; // [-1..1]  -1=Left, 0=Center, 1=Right
  float anchor_ndc_y = 0.0f; // [-1..1]  -1=Bottom, 0=Center, 1=Top (Y is UP in NDC/Layout)
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
    
    // Convert Layout (NDC anchors + Pixel offsets) -> Absolute Pixels (Center)
    // Coords: Top-Left (0,0), +Y Down.
    // Anchors: -1=Left, 1=Right, -1=Bottom, 1=Top.
    
    float base_x = (anchor_ndc_x + 1.0f) * 0.5f * (float)this->view_w;
    // Y-flip: 1.0(Top) -> 0.0, -1.0(Bottom) -> H
    float base_y = (1.0f - anchor_ndc_y) * 0.5f * (float)this->view_h;

    // Apply offsets (+Y is Up in layout, so subtract from Pixel Y which is Down)
    x = base_x + offset_px_x;
    y = base_y - offset_px_y;

    if (size_px_w > 0.0f) w = size_px_w;
    if (size_px_h > 0.0f) h = size_px_h;

    // Automatic Pivot adjustment based on Anchor
    // Left (-1): Pivot Left (x += w/2)
    // Right (1): Pivot Right (x -= w/2)
    // Top (1): Pivot Top (y += h/2)
    // Bottom (-1): Pivot Bottom (y -= h/2)
    
    if (anchor_ndc_x < -0.9f) x += w * 0.5f;
    else if (anchor_ndc_x > 0.9f) x -= w * 0.5f;
    
    if (anchor_ndc_y > 0.9f) y += h * 0.5f;
    else if (anchor_ndc_y < -0.9f) y -= h * 0.5f;
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


} } // namespace shine::ui

