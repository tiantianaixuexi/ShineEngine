#ifndef SHINE_GAME_SPRITE_RENDERER_H
#define SHINE_GAME_SPRITE_RENDERER_H

// game/sprite_renderer.h
// Minimal render component that emits draw commands via RenderContext.

#include "transform.h"

namespace shine::game {

class SpriteRenderer final : public Component {
public:
  int texId = 0; // 0 => draw colored
  float r = 1.0f;
  float g = 1.0f;
  float b = 1.0f;
  int transparent = 0;
  int depthOnly = 0;
  int emissive = 0;

  inline void setTransparent(bool v) noexcept { transparent = v ? 1 : 0; }
  inline void setDepthOnly(bool v) noexcept { depthOnly = v ? 1 : 0; }
  inline void setEmissive(bool v) noexcept { emissive = v ? 1 : 0; }

  void onRender(RenderContext& rc, float /*t*/) override {
    if (!node) return;
    bool shouldRender = false;
    switch (rc.pass) {
      case renderer::PASS_DEPTH: shouldRender = depthOnly; break;
      case renderer::PASS_OPAQUE:
      case renderer::PASS_BASE_COLOR:
      case renderer::PASS_UNLIT: shouldRender = !(transparent || emissive); break;
      case renderer::PASS_TRANSPARENT: shouldRender = transparent; break;
      case renderer::PASS_EMISSIVE: shouldRender = emissive; break;
      default: break;
    }
    if (!shouldRender) return;

    Transform* tr = node->getComponent<Transform>();
    if (!tr) return;

    float cx, cy, cz;
    tr->worldXYZ(cx, cy, cz);
    const float w = tr->w;
    const float h = tr->h;
    const Rect rect{cx, cy, w, h};
    const Color4 color{r, g, b, 1.0f};

    // SortKey layout: [layer:8][material:8][depth:16]
    unsigned int layer = rc.sortKey & 0xFFu;
    unsigned int mat = (unsigned int)(texId & 0xFF);
    float nz = (cz + 1.0f) * 0.5f;
    if (nz < 0.0f) nz = 0.0f;
    if (nz > 1.0f) nz = 1.0f;
    unsigned int depthKey = (unsigned int)(nz * 65535.0f);
    if (transparent) depthKey = 65535u - depthKey;
    unsigned int sortKey = (layer << 24) | (mat << 16) | depthKey;

    if (rc.drawRectTexEx && texId != 0) {
      rc.drawRectTexEx(rc.user, texId, rect, sortKey);
    } else if (rc.drawRectColEx) {
      rc.drawRectColEx(rc.user, rect, color, sortKey);
    } else if (texId != 0 && rc.drawRectTex) {
      rc.drawRectTex(rc.user, texId, rect);
    } else if (rc.drawRectCol) {
      rc.drawRectCol(rc.user, rect, color);
    }
  }
};

} // namespace shine::game

#endif // SHINE_GAME_SPRITE_RENDERER_H


