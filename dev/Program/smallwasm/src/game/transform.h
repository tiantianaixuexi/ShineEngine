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
  float z = 0.0f;
  float w = 0.25f;
  float h = 0.25f;

  inline void markDirty() noexcept {
    if (!m_dirty) {
      m_dirty = true;
      if (node) markChildrenDirty(node);
    }
  }

  inline void setPosition(float x_, float y_, float z_ = 0.0f) noexcept {
    x = x_;
    y = y_;
    z = z_;
    markDirty();
  }

  inline void setSize(float w_, float h_) noexcept {
    w = w_;
    h = h_;
    markDirty();
  }

  inline void worldXY(float& outX, float& outY) const noexcept {
    float outZ = 0.0f;
    worldXYZ(outX, outY, outZ);
  }

  inline void worldXYZ(float& outX, float& outY, float& outZ) const noexcept {
    Node* p = node ? node->parent : nullptr;
    float parentX = 0.0f;
    float parentY = 0.0f;
    float parentZ = 0.0f;
    if (p) {
      Transform* pt = p->getComponent<Transform>();
      if (pt) pt->worldXYZ(parentX, parentY, parentZ);
    }

    if (!m_dirty) {
      if (p != m_lastParent ||
          x != m_lastX || y != m_lastY || z != m_lastZ ||
          parentX != m_lastParentX || parentY != m_lastParentY || parentZ != m_lastParentZ) {
        m_dirty = true;
      }
    }

    if (m_dirty) {
      m_worldX = x + parentX;
      m_worldY = y + parentY;
      m_worldZ = z + parentZ;
      m_lastX = x; m_lastY = y; m_lastZ = z;
      m_lastParentX = parentX; m_lastParentY = parentY; m_lastParentZ = parentZ;
      m_lastParent = p;
      m_dirty = false;
    }

    outX = m_worldX;
    outY = m_worldY;
    outZ = m_worldZ;
  }

private:
  mutable float m_worldX = 0.0f;
  mutable float m_worldY = 0.0f;
  mutable float m_worldZ = 0.0f;
  mutable float m_lastX = 0.0f;
  mutable float m_lastY = 0.0f;
  mutable float m_lastZ = 0.0f;
  mutable float m_lastParentX = 0.0f;
  mutable float m_lastParentY = 0.0f;
  mutable float m_lastParentZ = 0.0f;
  mutable Node* m_lastParent = nullptr;
  mutable bool m_dirty = true;

  static void markChildrenDirty(Node* n) noexcept {
    for (unsigned int i = 0; i < n->children.size(); ++i) {
      Node* child = n->children[i];
      if (!child) continue;
      if (Transform* tr = child->getComponent<Transform>()) {
        tr->m_dirty = true;
      }
      markChildrenDirty(child);
    }
  }
};

inline void markTransformDirty(Node* n) noexcept {
  if (!n) return;
  if (Transform* tr = n->getComponent<Transform>()) tr->markDirty();
}

} // namespace shine::game

#endif // SHINE_GAME_TRANSFORM_H

