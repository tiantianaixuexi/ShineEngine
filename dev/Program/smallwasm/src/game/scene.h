#ifndef SHINE_GAME_SCENE_H
#define SHINE_GAME_SCENE_H

// game/scene.h
// Scene owns ONLY the Node hierarchy. (No components live in Scene directly.)

#include "node.h"

namespace shine { namespace game {

class Scene {
public:
  Node root{"Root"};
  unsigned int gc_budget = 64;
  unsigned int gc_interval = 8;
  unsigned int gc_frame = 0;

  inline void update(float t) noexcept {
      root.update(t);
  }

  inline void render(RenderContext& rc, float t) noexcept {
      root.renderTree(rc, t);
  }

  inline void pointer(float x_ndc, float y_ndc, int isDown) noexcept {
      root.pointerTree(x_ndc, y_ndc, isDown);
  }

  // Very small GC:
  // - Mark everything reachable from Scene.root
  // - Sweep the global Object registry:
  //   delete objects that are pendingKill or unreachable (unmarked)
  inline void collectGarbage() noexcept {
    if (++gc_frame < gc_interval) return;
    gc_frame = 0;
    if (Object::pendingKillCount() == 0u && gc_phase == 0u) return;
    collectGarbageIncremental(gc_budget);
  }

  inline void collectGarbageIncremental(unsigned int budget) noexcept {
    if (budget == 0u) return;
    unsigned int steps = budget;
    if (gc_phase == 0u) {
      gc_cursor = Object::gcHead();
      gc_phase = 1u;
    }

    while (steps > 0u && gc_phase == 1u) {
      if (!gc_cursor) {
        gc_phase = 2u;
        break;
      }
      Object* next = gc_cursor->gcNext();
      gc_cursor->gcUnmark();
      gc_cursor = next;
      --steps;
    }
    if (steps == 0u) return;

    if (gc_phase == 2u) {
      root.markTree();
      gc_cursor = Object::gcHead();
      gc_phase = 3u;
    }

    while (steps > 0u && gc_phase == 3u) {
      if (!gc_cursor) {
        gc_phase = 0u;
        return;
      }
      Object* next = gc_cursor->gcNext();
      const bool dead = gc_cursor->pendingKill() || !gc_cursor->gcMarked();
      if (dead) {
        if (!gc_cursor->isOwnedByDead()) {
          delete gc_cursor;
        }
      }
      gc_cursor = next;
      --steps;
    }
  }

private:
  Object* gc_cursor = nullptr;
  unsigned int gc_phase = 0;
};

} } // namespace shine::game

#endif // SHINE_GAME_SCENE_H

