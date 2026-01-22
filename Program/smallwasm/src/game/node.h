#ifndef SHINE_GAME_NODE_H
#define SHINE_GAME_NODE_H

// game/node.h
// Node tree + component mounting.
// IMPORTANT:
// - Node children define the Scene hierarchy (Scene contains only Nodes).
// - Components are mounted onto Nodes.
// - Components may have sub-components, which is NOT the same as Node child.

#include "../Container/SVector.h"
#include "component.h"
#include "object.h"

namespace shine::game {

class Node : public Object {
public:
  Node* parent = nullptr;

  // Scene hierarchy (Node children)
  wasm::SVector<Node*> children;

  // Components mounted on THIS node
  wasm::SVector<Component*> components;

  explicit Node(const char* debugName = nullptr) noexcept : Object(debugName) {}
  virtual ~Node() {
        // 1. Destroy components (Forward order)
        for (auto* c : components) {
            if (c) {
                c->node = nullptr; // Prevent removeComponent
                c->parent = nullptr;
                delete c;
            }
        }
        components.clear();
  
        // 2. Destroy children (Forward order)
        for (auto* n : children) {
            if (n) {
                n->parent = nullptr; // Prevent removeChild
                delete n;
            }
        }
        children.clear();
  
        // 3. Detach from parent
        if (parent) parent->removeChild(this);
    }
  ObjectKind kind() const noexcept override { return ObjectKind::Node; }

  // GC Helpers
  bool isOwnedByDead() const override {
      return parent && (parent->pendingKill() || !parent->gcMarked());
  }

  // ---- Node child ops (Scene graph) ----
  inline void attachChild(Node* n) noexcept {
    if (!n) return;
    n->parent = this;
    children.push_back(n);
  }

  inline void removeChild(Node* n) noexcept {
    if (!n) return;
    children.erase_first_unordered(n);
  }

  template<typename T, typename... Args>
  inline T* addChildNode(Args&&... args) noexcept {
    T* n = new T((Args&&)args...);
    attachChild(n);
    return n;
  }

  // ---- Component ops (mounted to Node) ----
  inline void attachComponent(Component* c) noexcept {
    if (!c) return;
    c->node = this;
    c->parent = nullptr;
    components.push_back(c);
    c->onAttach();
  }

  inline void removeComponent(Component* c) noexcept {
    if (!c) return;
    components.erase_first_unordered(c);
  }

  inline void markTree() noexcept {
    gcMark();
    for (unsigned int i = 0; i < components.size(); ++i) {
      Component* c = components[i];
      if (c) c->markTree();
    }
    for (unsigned int i = 0; i < children.size(); ++i) {
      Node* n = children[i];
      if (n) n->markTree();
    }
  }

  template<typename T, typename... Args>
  inline T* addComponent(Args&&... args) noexcept {
    T* c = new T((Args&&)args...);
    c->template setTypeId<T>(); // Auto-register type ID
    attachComponent(c);
    return c;
  }

  template<typename T>
  inline T* getComponent() noexcept {
    const ComponentTypeId want = ComponentType<T>::id();
    for (unsigned int i = 0; i < components.size(); ++i) {
      Component* c = components[i];
      if (c && c->typeId() == want) return (T*)c;
    }
    return nullptr;
  }

  // ---- traversal ----
  inline void update(float t) noexcept {
    if (!isActive()) return; // Node active gates the whole subtree
    for (unsigned int i = 0; i < components.size(); ++i) {
      Component* c = components[i];
      if (c) c->update(t);
    }
    for (unsigned int i = 0; i < children.size(); ++i) {
      Node* n = children[i];
      if (n) n->update(t);
    }
  }

  inline void renderTree(RenderContext& rc, float t) noexcept {
    if (!isActive() || !isVisible()) return; // Node visibility gates the whole subtree
    for (unsigned int i = 0; i < components.size(); ++i) {
      Component* c = components[i];
      if (c) c->renderTree(rc, t);
    }
    for (unsigned int i = 0; i < children.size(); ++i) {
      Node* n = children[i];
      if (n) n->renderTree(rc, t);
    }
  }

  inline void pointerTree(float x_ndc, float y_ndc, int isDown) noexcept {
    if (!isActive()) return;
    for (unsigned int i = 0; i < components.size(); ++i) {
      Component* c = components[i];
      if (c) c->pointerTree(x_ndc, y_ndc, isDown);
    }
    for (unsigned int i = 0; i < children.size(); ++i) {
      Node* n = children[i];
      if (n) n->pointerTree(x_ndc, y_ndc, isDown);
    }
  }
};

// Implement Component methods that require Node definition
// inline void Component::detach() ... REMOVED

inline bool Component::isOwnedByDead() const {
    return (parent && (parent->pendingKill() || !parent->gcMarked())) ||
           (node && (node->pendingKill() || !node->gcMarked()));
}

inline Component::~Component() {
      // 1. Destroy children (Forward order)
      for (auto* c : children) {
          if (c) {
              c->parent = nullptr; // Prevent child from calling removeChild on us
              c->node = nullptr;
              delete c;
          }
      }
      children.clear();
  
      // 2. Detach from parent
      onDetach();
      if (parent) parent->removeChild(this);
      else if (node) node->removeComponent(this);
  }

}// namespace shine::game

#endif // SHINE_GAME_NODE_H

