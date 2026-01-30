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
    m_stack_update.clear();
    if (m_stack_update.capacity() < 64u) m_stack_update.reserve(64u);
    m_stack_update.push_back(&root);
    while (!m_stack_update.empty()) {
      Node* n = m_stack_update.back();
      m_stack_update.pop_back();
      if (unlikely(!n || !n->isActive())) continue;
      for (unsigned int i = 0; i < n->components.size(); ++i) {
        Component* c = n->components[i];
        if (c) c->update(t);
      }
      for (unsigned int i = 0; i < n->children.size(); ++i) {
        Node* c = n->children[i];
        if (c) m_stack_update.push_back(c);
      }
    }
  }

  inline void render(RenderContext& rc, float t) noexcept {
    m_stack_render.clear();
    if (m_stack_render.capacity() < 64u) m_stack_render.reserve(64u);
    m_stack_render.push_back(&root);
    while (!m_stack_render.empty()) {
      Node* n = m_stack_render.back();
      m_stack_render.pop_back();
      if (unlikely(!n || !n->isActive())) continue;
      for (unsigned int i = 0; i < n->components.size(); ++i) {
        Component* c = n->components[i];
        if (c) c->renderTree(rc, t);
      }
      for (unsigned int i = 0; i < n->children.size(); ++i) {
        Node* c = n->children[i];
        if (c) m_stack_render.push_back(c);
      }
    }
  }

  inline void pointer(float x_ndc, float y_ndc, int isDown) noexcept {
    m_stack_pointer.clear();
    if (m_stack_pointer.capacity() < 64u) m_stack_pointer.reserve(64u);
    m_stack_pointer.push_back(&root);
    while (!m_stack_pointer.empty()) {
      Node* n = m_stack_pointer.back();
      m_stack_pointer.pop_back();
      if (unlikely(!n || !n->isActive())) continue;
      for (unsigned int i = 0; i < n->components.size(); ++i) {
        Component* c = n->components[i];
        if (c) c->pointerTree(x_ndc, y_ndc, isDown);
      }
      for (unsigned int i = 0; i < n->children.size(); ++i) {
        Node* c = n->children[i];
        if (c) m_stack_pointer.push_back(c);
      }
    }
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
  shine::wasm::SVector<Node*> m_stack_update;
  shine::wasm::SVector<Node*> m_stack_render;
  shine::wasm::SVector<Node*> m_stack_pointer;
};

} } // namespace shine::game

#endif // SHINE_GAME_SCENE_H

