#pragma once

#include "wasm_compat.h"

extern "C" {
  void js_console_log_int(const char* label, int labelLen, int val);
  void js_console_log(const char* msg, int len);
}

// Simple Logger
static inline void LOG(const char* msg) {
    if (msg) {
        int len = 0;
        while(msg[len]) len++;
        js_console_log(msg, len);
    }
}

static inline void LOG(const char* label, int val) {
    if (label) {
        int len = 0;
        while(label[len]) len++;
        js_console_log_int(label, len, val);
    }
}
