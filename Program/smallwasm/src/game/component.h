#ifndef SHINE_GAME_COMPONENT_H
#define SHINE_GAME_COMPONENT_H

// game/component.h
// Component base + "sub-component" hierarchy.
// IMPORTANT: Component children are NOT Node children.

#include "../Container/SVector.h"
#include "render_context.h"
#include "object.h"

namespace shine::game {

class Node;

using ComponentTypeId = const void*;

// Helper for generic type-id generation.
template<typename T>
struct ComponentType {
  static inline ComponentTypeId id() noexcept {
    static int k;
    return (ComponentTypeId)&k;
  }
};

class Component : public Object {
public:
  Node* node = nullptr; // owner Node (null until attached)

  // RTTI replacement for -fno-rtti builds
  ComponentTypeId m_typeId = nullptr;

  template<typename T>
  inline void setTypeId() noexcept {
    m_typeId = ComponentType<T>::id();
  }

  inline ComponentTypeId typeId() const noexcept {
    return m_typeId;
  }

  // Component sub-tree (NOT Scene/Node hierarchy).
  shine::wasm::SVector<Component*> children;
  Component* parent = nullptr;

  explicit Component(const char* debugName = nullptr) noexcept : Object(debugName) {}
  virtual ~Component() = default;
  ObjectKind kind() const noexcept override { return ObjectKind::Component; }

  // ---- lifecycle ----
  virtual void onAttach() {}
  virtual void onDetach() {}

  // ---- update/render hooks ----
  virtual void onUpdate(float /*t*/) {}
  virtual void onRender(RenderContext& /*rc*/, float /*t*/) {}
  virtual void onPointer(float /*x_ndc*/, float /*y_ndc*/, int /*isDown*/) {}

  // ---- component-child ops (sub-components) ----
  inline void attachChild(Component* c) noexcept {
    if (!c) return;
    c->parent = this;
    c->node = node; // inherits Node binding
    children.push_back(c);
    c->onAttach();
  }

  inline void removeChild(Component* c) noexcept {
    if (!c) return;
    children.erase_first_unordered(c);
  }

  inline void markTree() noexcept {
    gcMark();
    for (unsigned int i = 0; i < children.size(); ++i) {
      Component* c = children[i];
      if (c) c->markTree();
    }
  }

  inline void update(float t) noexcept {
    if (!isActive()) return;
    if (tickEnabled()) onUpdate(t);
    for (unsigned int i = 0; i < children.size(); ++i) {
      Component* c = children[i];
      if (c) c->update(t);
    }
  }

  inline void renderTree(RenderContext& rc, float t) noexcept {
    if (!isActive()) return;
    if (isVisible() && renderEnabled()) onRender(rc, t);
    for (unsigned int i = 0; i < children.size(); ++i) {
      Component* c = children[i];
      if (c) c->renderTree(rc, t);
    }
  }

  inline void pointerTree(float x_ndc, float y_ndc, int isDown) noexcept {
    if (!isActive()) return;
    if (pointerEnabled()) onPointer(x_ndc, y_ndc, isDown);
    for (unsigned int i = 0; i < children.size(); ++i) {
      Component* c = children[i];
      if (c) c->pointerTree(x_ndc, y_ndc, isDown);
    }
  }

  inline void destroyTree() noexcept {
    for (unsigned int i = 0; i < children.size(); ++i) {
      Component* c = children[i];
      if (c) {
        c->destroyTree();
        delete c;
      }
    }
    children.clear();
    onDetach();
  }
};

} // namespace shine::game

#endif // SHINE_GAME_COMPONENT_H

