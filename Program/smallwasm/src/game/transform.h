#ifndef SHINE_GAME_TRANSFORM_H
#define SHINE_GAME_TRANSFORM_H

// game/transform.h
// Simple 2D transform (NDC space for now).

#include "node.h"

namespace shine::game {

class Transform final : public Component {
public:
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.25f;
  float h = 0.25f;

  // Compute world position by walking Node parents and accumulating their Transform.
  inline void worldXY(float& outX, float& outY) const noexcept {
    outX = x;
    outY = y;
    Node* p = node ? node->parent : nullptr;
    while (p) {
      Transform* pt = p->getComponent<Transform>();
      if (pt) { outX += pt->x; outY += pt->y; }
      p = p->parent;
    }
  }
};

} // namespace shine::game

#endif // SHINE_GAME_TRANSFORM_H

