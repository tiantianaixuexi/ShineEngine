#ifndef SHINE_GAME_RENDER_CONTEXT_H
#define SHINE_GAME_RENDER_CONTEXT_H

// game/render_context.h
// Minimal render API passed into components (no STL).
// Components should not know about DemoApp/WebGL; they only call these callbacks.

namespace shine { namespace game {

struct RenderContext {
  void* user = nullptr;

  // Draw a colored rect (centered at cx,cy) in NDC units.
  void (*drawRectCol)(void* user, float cx, float cy, float w, float h, float r, float g, float b) = nullptr;

  // Draw a textured rect (centered at cx,cy) in NDC units.
  void (*drawRectTex)(void* user, int texId, float cx, float cy, float w, float h) = nullptr;
};

} } // namespace shine::game

#endif // SHINE_GAME_RENDER_CONTEXT_H

