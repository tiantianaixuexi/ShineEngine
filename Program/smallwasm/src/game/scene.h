#ifndef SHINE_GAME_SCENE_H
#define SHINE_GAME_SCENE_H

// game/scene.h
// Scene owns ONLY the Node hierarchy. (No components live in Scene directly.)

#include "node.h"

namespace shine { namespace game {

class Scene {
public:
  Node root{"Root"};

  inline void update(float t) noexcept { root.update(t); }
  inline void render(RenderContext& rc, float t) noexcept { root.renderTree(rc, t); }
  inline void pointer(float x_ndc, float y_ndc, int isDown) noexcept { root.pointerTree(x_ndc, y_ndc, isDown); }

  // Very small GC:
  // - Mark everything reachable from Scene.root
  // - Sweep the global Object registry:
  //   delete objects that are pendingKill or unreachable (unmarked)
  inline void collectGarbage() noexcept {
    // 1) unmark all
    for (Object* o = Object::gcHead(); o; o = o->gcNext()) o->gcUnmark();

    // 2) mark from root
    root.markTree();

    // 3) sweep: delete "dead roots" only (avoid double-delete of subtrees)
    for (Object* o = Object::gcHead(); o; ) {
      Object* next = o->gcNext(); // cache before possible delete

      const bool dead = o->pendingKill() || !o->gcMarked();
      if (dead) {
        if (o->kind() == ObjectKind::Node) {
          Node* n = (Node*)o;
          // If parent is also dead, parent deletion will cover us.
          const bool parentDead = (n->parent && (n->parent->pendingKill() || !n->parent->gcMarked()));
          if (!parentDead) {
            if (n->parent) n->parent->removeChild(n);
            n->destroyTree();
            delete n;
          }
        } else { // Component
          Component* c = (Component*)o;
          const bool parentDead = (c->parent && (c->parent->pendingKill() || !c->parent->gcMarked()));
          const bool nodeDead = (c->node && (c->node->pendingKill() || !c->node->gcMarked()));
          if (!parentDead && !nodeDead) {
            if (c->parent) c->parent->removeChild(c);
            else if (c->node) c->node->removeComponent(c);
            c->destroyTree();
            delete c;
          }
        }
      }

      o = next;
    }
  }
};

} } // namespace shine::game

#endif // SHINE_GAME_SCENE_H

