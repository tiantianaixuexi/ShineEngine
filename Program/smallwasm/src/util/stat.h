#pragma once

// util/stat.h
// Debug-only stats reporting from WASM to JS (optional).
//
// Usage:
//   STAT_SET_F32("Gameplay/AI/TickMs", tickMs, "ms");
//   STAT_SET_I32("Render/CmdCount", cmdCount, "");
//
// Notes:
// - Compiled out in non-DEBUG builds.
// - JS side is a no-op unless stats_debug.js is loaded (press ` or add ?stat=1).

#include "wasm_compat.h"

namespace shine::stat {

#ifdef DEBUG
extern "C" {
  // wasm32: pointers passed as i32 offsets
  void js_stat_f32(int name_ptr, int name_len, float value, int unit_ptr, int unit_len);
  void js_stat_i32(int name_ptr, int name_len, int value, int unit_ptr, int unit_len);
}

static inline void set_f32(const char* name, float value, const char* unit = "") noexcept {
  if (!name) return;
  if (!unit) unit = "";
  js_stat_f32(
    shine::wasm::ptr_i32(name),
    shine::wasm::raw_strlen(name),
    value,
    shine::wasm::ptr_i32(unit),
    shine::wasm::raw_strlen(unit)
  );
}

static inline void set_i32(const char* name, int value, const char* unit = "") noexcept {
  if (!name) return;
  if (!unit) unit = "";
  js_stat_i32(
    shine::wasm::ptr_i32(name),
    shine::wasm::raw_strlen(name),
    value,
    shine::wasm::ptr_i32(unit),
    shine::wasm::raw_strlen(unit)
  );
}

#else
static inline void set_f32(const char*, float, const char* = "") noexcept {}
static inline void set_i32(const char*, int, const char* = "") noexcept {}
#endif

} // namespace shine::stat

#define STAT_SET_F32(name, value, unit) ::shine::stat::set_f32((name), (float)(value), (unit))
#define STAT_SET_I32(name, value, unit) ::shine::stat::set_i32((name), (int)(value), (unit))

