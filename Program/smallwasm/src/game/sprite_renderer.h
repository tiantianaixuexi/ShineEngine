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

  void onRender(RenderContext& rc, float /*t*/) override {
    if (!node) return;
    Transform* tr = node->getComponent<Transform>();
    if (!tr) return;

    float cx, cy;
    tr->worldXY(cx, cy);
    const float w = tr->w;
    const float h = tr->h;

    if (texId != 0) {
      if (rc.drawRectTex) rc.drawRectTex(rc.user, texId, cx, cy, w, h);
    } else {
      if (rc.drawRectCol) rc.drawRectCol(rc.user, cx, cy, w, h, r, g, b);
    }
  }
};

} // namespace shine::game

#endif // SHINE_GAME_SPRITE_RENDERER_H


