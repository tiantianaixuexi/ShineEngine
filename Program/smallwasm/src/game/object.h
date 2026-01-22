#ifndef SHINE_GAME_OBJECT_H
#define SHINE_GAME_OBJECT_H

// game/object.h
// Shared base for Node and Component:
// - id / debug name
// - flags: active, visible, tick enabled, render enabled, pointer enabled
// - minimal GC hooks: mark bit + pending kill
//
// NOTE: This is header-only and uses a TU-local id counter (function-static).
// For this small wasm demo it is sufficient; if you later split into many TUs,
// consider moving the counter to a single .cpp.

namespace shine { namespace game {

enum ObjectFlags : unsigned int {
  OF_Active      = 1u << 0,
  OF_Visible     = 1u << 1,
  OF_Tick        = 1u << 2,
  OF_Render      = 1u << 3,
  OF_Pointer     = 1u << 4,
  OF_PendingKill = 1u << 5,
  OF_GCMark      = 1u << 6,
};

enum class ObjectKind : unsigned char {
  Node = 1,
  Component = 2,
};

class Object {
public:
  unsigned int id = 0;
  const char* name = nullptr;
  unsigned int flags = (OF_Active | OF_Visible | OF_Tick | OF_Render | OF_Pointer);

  explicit Object(const char* debugName = nullptr) noexcept
    : id(next_id()), name(debugName) {
    gc_link();
  }

  virtual ~Object() { gc_unlink(); }

  virtual ObjectKind kind() const noexcept = 0;
  virtual bool isOwnedByDead() const { return false; }

  inline bool isActive() const noexcept { return (flags & OF_Active) != 0; }
  inline bool isVisible() const noexcept { return (flags & OF_Visible) != 0; }
  inline bool tickEnabled() const noexcept { return (flags & OF_Tick) != 0; }
  inline bool renderEnabled() const noexcept { return (flags & OF_Render) != 0; }
  inline bool pointerEnabled() const noexcept { return (flags & OF_Pointer) != 0; }
  inline bool pendingKill() const noexcept { return (flags & OF_PendingKill) != 0; }

  inline void setActive(bool v) noexcept { flags = v ? (flags | OF_Active) : (flags & ~OF_Active); }
  inline void setVisible(bool v) noexcept { flags = v ? (flags | OF_Visible) : (flags & ~OF_Visible); }
  inline void setTickEnabled(bool v) noexcept { flags = v ? (flags | OF_Tick) : (flags & ~OF_Tick); }
  inline void setRenderEnabled(bool v) noexcept { flags = v ? (flags | OF_Render) : (flags & ~OF_Render); }
  inline void setPointerEnabled(bool v) noexcept { flags = v ? (flags | OF_Pointer) : (flags & ~OF_Pointer); }

  // GC hooks (optional for later):
  inline void gcMark() noexcept { flags |= OF_GCMark; }
  inline void gcUnmark() noexcept { flags &= ~OF_GCMark; }
  inline bool gcMarked() const noexcept { return (flags & OF_GCMark) != 0; }

  // Lifetime hint (optional for later):
  inline void markPendingKill() noexcept { flags |= OF_PendingKill; }

  // ---- GC registry (very small & simple) ----
  static inline Object* gcHead() noexcept { return s_gc_head; }
  inline Object* gcNext() const noexcept { return gc_next; }

private:
  // intrusive global list
  Object* gc_prev = nullptr;
  Object* gc_next = nullptr;
  static Object* s_gc_head;

  inline void gc_link() noexcept {
    // Insert at head.
    gc_prev = nullptr;
    gc_next = s_gc_head;
    if (s_gc_head) s_gc_head->gc_prev = this;
    s_gc_head = this;
  }

  inline void gc_unlink() noexcept {
    if (gc_prev) gc_prev->gc_next = gc_next;
    else if (s_gc_head == this) s_gc_head = gc_next;
    if (gc_next) gc_next->gc_prev = gc_prev;
    gc_prev = nullptr;
    gc_next = nullptr;
  }

  static inline unsigned int next_id() noexcept {
    static unsigned int s = 1;
    unsigned int v = s++;
    if (s == 0) s = 1; // avoid 0
    return v;
  }
};

// NOTE:
// We intentionally do NOT provide an out-of-class definition here to avoid
// requiring C++17 inline variables. Provide the definition in a single .cpp if needed.

} } // namespace shine::game

#endif // SHINE_GAME_OBJECT_H

