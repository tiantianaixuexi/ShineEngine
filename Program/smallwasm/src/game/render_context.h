#ifndef SHINE_GAME_RENDER_CONTEXT_H
#define SHINE_GAME_RENDER_CONTEXT_H

// game/render_context.h
// Minimal render API passed into components (no STL).
// Components should not know about DemoApp/WebGL; they only call these callbacks.

#include "../renderer/render_pipeline.h"
#include "../math/Rect.h"
#include "../math/Color4.h"

namespace shine { namespace game {

using shine::math::Rect;
using shine::math::Color4;

struct RenderContext {
  using DrawRectColFn = void (*)(void* user, const Rect& rect, const Color4& color);
  using DrawRectTexFn = void (*)(void* user, int texId, const Rect& rect);
  using DrawRectColExFn = void (*)(void* user, const Rect& rect, const Color4& color, unsigned int sortKey);
  using DrawRectTexExFn = void (*)(void* user, int texId, const Rect& rect, unsigned int sortKey);

  void* user = nullptr;
  renderer::RenderPassType pass = renderer::PASS_BASE_COLOR;
  unsigned int sortKey = 0;

  // Draw a colored rect (centered at cx,cy) in NDC units.
  DrawRectColFn drawRectCol = nullptr;

  // Draw a textured rect (centered at cx,cy) in NDC units.
  DrawRectTexFn drawRectTex = nullptr;

  // Optional: sorted variants for transparent ordering.
  DrawRectColExFn drawRectColEx = nullptr;
  DrawRectTexExFn drawRectTexEx = nullptr;
};

} } // namespace shine::game

#endif // SHINE_GAME_RENDER_CONTEXT_H

