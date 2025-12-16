#pragma once

// util/timer.h
// A tiny timer queue for wasm(-nostdlib) builds.
//
// Design:
// - No OS timer threads. You drive it by calling tick(nowSeconds) once per frame.
// - No STL/libc dependency. Uses shine::wasm::raw_malloc/raw_free.
// - Cancellation is "mark + sweep" to be safe during callbacks.
//
// Typical usage:
//   static shine::util::TimerQueue g_timers;
//   g_timers.init(0.0f);
//   g_timers.after(1.0f, cb, user);      // fire once after 1s
//   g_timers.every(0.25f, cb, user);     // fire every 0.25s
//   g_timers.tick(t_seconds);           // call each frame

#include "wasm_compat.h"

namespace shine::util {

using TimerId = unsigned int;
using TimerCallback = void (*)(TimerId id, void* user);

struct TimerTask {
  TimerId id = 0;
  float due = 0.0f;       // absolute time in seconds
  float interval = 0.0f;  // seconds (for repeating timers)
  TimerCallback cb = nullptr;
  void* user = nullptr;
  unsigned char repeat = 0;
  unsigned char cancelled = 0;
  TimerTask* next = nullptr;
};

struct TimerQueue {
  TimerTask* head = nullptr;
  TimerId nextId = 1;
  float now = 0.0f;

  inline void init(float nowSeconds = 0.0f) noexcept {
    clear();
    now = nowSeconds;
    if (nextId == 0) nextId = 1;
  }

  inline void clear() noexcept {
    TimerTask* t = head;
    head = nullptr;
    while (t) {
      TimerTask* n = t->next;
      shine::wasm::raw_free(t);
      t = n;
    }
  }

  inline TimerId after(float delaySeconds, TimerCallback cb, void* user = nullptr) noexcept {
    if (!cb) return 0;
    if (delaySeconds < 0.0f) delaySeconds = 0.0f;
    TimerTask* t = (TimerTask*)shine::wasm::raw_malloc((shine::wasm::size_t)sizeof(TimerTask));
    if (!t) return 0;
    *t = TimerTask{};
    t->id = alloc_id();
    t->due = now + delaySeconds;
    t->interval = 0.0f;
    t->cb = cb;
    t->user = user;
    t->repeat = 0;
    t->cancelled = 0;
    t->next = head;
    head = t;
    return t->id;
  }

  // Repeating timer. If firstDelaySeconds < 0, the first fire happens after intervalSeconds.
  inline TimerId every(float intervalSeconds, TimerCallback cb, void* user = nullptr, float firstDelaySeconds = -1.0f) noexcept {
    if (!cb) return 0;
    if (intervalSeconds <= 0.0f) intervalSeconds = 0.001f;
    if (firstDelaySeconds < 0.0f) firstDelaySeconds = intervalSeconds;
    if (firstDelaySeconds < 0.0f) firstDelaySeconds = 0.0f;
    TimerTask* t = (TimerTask*)shine::wasm::raw_malloc((shine::wasm::size_t)sizeof(TimerTask));
    if (!t) return 0;
    *t = TimerTask{};
    t->id = alloc_id();
    t->due = now + firstDelaySeconds;
    t->interval = intervalSeconds;
    t->cb = cb;
    t->user = user;
    t->repeat = 1;
    t->cancelled = 0;
    t->next = head;
    head = t;
    return t->id;
  }

  // Safe to call from inside callbacks.
  inline void cancel(TimerId id) noexcept {
    if (id == 0) return;
    for (TimerTask* t = head; t; t = t->next) {
      if (t->id == id) {
        t->cancelled = 1;
        return;
      }
    }
  }

  // Drive timers to "nowSeconds". Executes callbacks at most once per timer per tick.
  inline void tick(float nowSeconds) noexcept {
    now = nowSeconds;

    TimerTask** pp = &head;
    while (*pp) {
      TimerTask* t = *pp;

      if (t->cancelled) {
        *pp = t->next;
        shine::wasm::raw_free(t);
        continue;
      }

      if (t->due <= now) {
        // Call user code. It may cancel timers (including itself).
        TimerCallback cb = t->cb;
        TimerId id = t->id;
        void* user = t->user;
        if (cb) cb(id, user);

        if (t->cancelled || !t->repeat) {
          *pp = t->next;
          shine::wasm::raw_free(t);
          continue;
        }

        // Reschedule repeating timer. Avoid "catch-up storm" if the frame hitches:
        // push next fire to now + interval when already overdue.
        float nextDue = t->due + t->interval;
        if (nextDue <= now) nextDue = now + t->interval;
        t->due = nextDue;
      }

      pp = &t->next;
    }
  }

private:
  inline TimerId alloc_id() noexcept {
    TimerId id = nextId++;
    if (nextId == 0) nextId = 1; // avoid returning 0
    if (id == 0) id = nextId++;
    return id;
  }
};

} // namespace shine::util

