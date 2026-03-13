---
name: "tracy-profiler-usage"
description: "Standardized usage guidelines and quick reference for integrating Tracy Profiler into engine code. Invoke when adding instrumentation, debugging performance, or analyzing GPU/memory allocations."
---

# Tracy Profiler Usage Guidelines

## Goal

Establish consistent and correct usage of Tracy Profiler across the codebase, ensuring minimal overhead when disabled and rich profiling data when enabled.

## Invoke When

- Adding new performance instrumentation (zones, frames, plots)
- Debugging concurrency issues (lock tracking)
- Profiling GPU workloads (Vulkan, OpenGL, Direct3D, etc.)
- Tracking memory allocations and leaks
- Analyzing frame pacing and asset loading
- Reviewing or refactoring existing profiling code

## Core Rules

1. **Enable globally** – `TRACY_ENABLE` must be defined for the whole project to activate profiling. All Tracy macros become no‑ops when it is undefined.
2. **Static strings for names** – Zone names, thread names, and message literals **must** have static lifetime (string literals or never‑freed buffers). Use the `_V` variants or pass explicit `size` for dynamic strings (Tracy copies them internally).
3. **Name threads early** – Call `tracy::SetThreadName` at the start of each thread function to improve trace readability.
4. **Zone scope** – Use `ZoneScoped` (or named variants) at the beginning of a scope. Do not manually create zone objects unless you need conditional activation.
5. **Lock annotations** – Replace standard mutex types with `TracyLockable` to automatically track lock contention and deadlocks.
6. **GPU context** – One GPU context per thread; call collect macros every frame.
7. **Avoid overhead in hot paths** – Even with `TRACY_ENABLE` off, macros are zero‑cost. When enabled, be mindful of deep call stacks (`depth` parameter) and frequent `ZoneText` calls.

## Quick Reference

### Basic Setup

| Macro / Function | Description |
|------------------|-------------|
| `TRACY_ENABLE` | Define globally to enable profiling. |
| `tracy::SetThreadName(name)` | Name the current thread (appears in timeline). |
| `tracy::SetThreadNameWithHint(name, groupHint)` | Name thread with grouping hint (`int32_t`). |

### Frame Marking

| Macro | Description |
|-------|-------------|
| `FrameMark` | Marks the end of a continuous frame (e.g., after `SwapBuffers`). |
| `FrameMarkNamed(name)` | Marks a named continuous frame set. |
| `FrameMarkStart(name)` | Start of a discontinuous frame. |
| `FrameMarkEnd(name)` | End of a discontinuous frame. |
| `FrameImage(image, width, height, offset, flip)` | Sends RGBA image data for the current frame. |

### Zone Instrumentation

#### Basic Zones

| Macro | Description |
|-------|-------------|
| `ZoneScoped` | Automatic zone with function/file/line. |
| `ZoneScopedC(color)` | Zone with custom color (`0xRRGGBB`). |
| `ZoneScopedN(name)` | Zone with custom name. |
| `ZoneScopedNC(name, color)` | Zone with name and color. |
| `ZoneText(text, size)` | Attach text to current zone. |
| `ZoneColor(color)` | Dynamically change zone color. |
| `ZoneValue(value)` | Attach a `uint64_t` value. |
| `ZoneName(text, size)` | Dynamically rename zone. |
| `ZoneIsActive` | Returns whether zone is active. |

#### Named Zones (for conditional activation)

| Macro | Description |
|-------|-------------|
| `ZoneNamed(var, active)` | Create a named zone variable; `active` (bool) controls enablement. |
| `ZoneNamedN(var, name, active)` | Named zone with custom name. |
| `ZoneNamedC(var, color, active)` | Named zone with color. |
| `ZoneNamedNC(var, name, color, active)` | Named zone with name and color. |
| `ZoneTextV(var, text, size)` | Attach text to named zone. |
| `ZoneColorV(var, color)` | Change named zone color. |
| `ZoneValueV(var, value)` | Attach value to named zone. |
| `ZoneNameV(var, text, size)` | Rename named zone. |

#### Transient Zones (for dynamically generated code)

| Macro | Description |
|-------|-------------|
| `ZoneTransient(var, active)` | Transient zone that copies source location to heap, safe for dynamic strings. |
| `ZoneTransientN(var, name, active)` | Named transient zone. |

### Lock Tracking

| Macro | Description |
|-------|-------------|
| `TracyLockable(type, var)` | Define a trackable mutex (replaces `type var`). |
| `TracyLockableN(type, var, description)` | Trackable mutex with description. |
| `LockableBase(type)` | Base for template parameters (e.g., `std::lock_guard`). |
| `LockMark(var)` | Optional annotation of lock holding location. |
| `TracySharedLockable(type, var)` | Trackable shared mutex. |
| `SharedLockableBase(type)` | Base for shared lock templates. |

### Message Logging

| Macro | Description |
|-------|-------------|
| `TracyMessage(text, size)` | Send a text message. |
| `TracyMessageL(text)` | Send a string literal message. |
| `TracyMessageC(text, size, color)` | Colored message. |
| `TracyMessageLC(text, color)` | Colored string literal. |

### Plotting Data

| Macro | Description |
|-------|-------------|
| `TracyPlot(name, value)` | Record a numeric value (integer or float). |
| `TracyPlotConfig(name, format, step, fill, color)` | Configure plot display (`format` is `tracy::PlotFormatType`). |

### Memory Profiling

| Macro | Description |
|-------|-------------|
| `TracyAlloc(ptr, size)` | Record allocation. |
| `TracyFree(ptr)` | Record deallocation. |
| `TracySecureAlloc(ptr, size)` | Secure allocation (for TLS destruction, etc.). |
| `TracySecureFree(ptr)` | Secure free. |
| `TracyAllocN(ptr, size, name)` | Record allocation in named memory pool. |
| `TracyFreeN(ptr, name)` | Record free in named memory pool. |

### GPU Profiling

#### OpenGL

| Macro | Description |
|-------|-------------|
| `TracyGpuContext` | Declare an OpenGL context (one per thread). |
| `TracyGpuContextName(name, size)` | Set context name. |
| `TracyGpuZone(name)` | Mark a GPU zone. |
| `TracyGpuZoneC(name, color)` | Colored GPU zone. |
| `TracyGpuCollect` | Collect GPU events (call once per frame). |

#### Vulkan

| Macro | Description |
|-------|-------------|
| `TracyVkContext(physdev, device, queue, cmdbuf)` | Create Vulkan context; returns `ctx`. |
| `TracyVkContextCalibrated(...)` | Create calibrated context. |
| `TracyVkDestroy(ctx)` | Destroy context. |
| `TracyVkContextName(ctx, name, size)` | Set context name. |
| `TracyVkZone(ctx, cmdbuf, name)` | Mark GPU zone. |
| `TracyVkZoneC(ctx, cmdbuf, name, color)` | Colored GPU zone. |
| `TracyVkCollect(ctx, cmdbuf)` | Collect events. |

#### Other Graphics APIs

Similar macros exist for Direct3D 11/12, Metal, OpenCL, CUDA. Refer to the official Tracy manual for details.

### Fiber / Coroutine Support

| Macro | Description |
|-------|-------------|
| `TracyFiberEnter(fiber)` | Enter a fiber (fiber is a unique string ID). |
| `TracyFiberLeave` | Leave the current fiber. |

### Call Stack Collection

Use `S`‑suffixed macros with a `depth` parameter:

| Macro | Description |
|-------|-------------|
| `ZoneScopedS(depth)` | Zone with call stack capture. |
| `ZoneScopedNS(name, depth)` | Named zone with call stack. |
| `ZoneScopedCS(color, depth)` | Colored zone with call stack. |
| `ZoneScopedNCS(name, color, depth)` | Name + color + call stack. |
| `TracyAllocS(ptr, size, depth)` | Allocation with call stack. |
| `TracyFreeS(ptr, depth)` | Free with call stack. |
| `TracyMessageS(text, size, depth)` | Message with call stack. |

### Utility

| Macro / Function | Description |
|------------------|-------------|
| `TracyIsConnected` | Returns whether a server is currently connected. |
| `TracyAppInfo(text, size)` | Set application information (version, environment, etc.). |
| `TracySourceCallbackRegister(callback, data)` | Register callback to provide source code. |
| `TracyParameterRegister(callback, data)` | Register callback for runtime parameter tweaks. |

## Usage Patterns

- **Zone placement**: Place `ZoneScoped` at the beginning of any function you want to measure. For parts of a function, use named zones or manual zone objects.
- **Conditional zones**: Use `ZoneNamed` with an `active` boolean to dynamically disable expensive instrumentation in release builds without recompiling.
- **Lock tracking**: Always use `TracyLockable` instead of raw mutexes to automatically visualize lock contention.
- **Memory profiling**: Pair every `TracyAlloc` with a matching `TracyFree` to detect leaks.
- **GPU zones**: Ensure `TracyGpuCollect` (or `TracyVkCollect`) is called every frame to flush GPU queries.

## Important Notes

- **String lifetime**: Zone names, message strings, and thread names **must** be static literals unless you use the `size` parameter (Tracy copies the data). Failure to do so will cause garbage in the profiler UI.
- **Conditional compilation**: All Tracy macros are no‑ops when `TRACY_ENABLE` is **not** defined. You can safely leave them in production code without performance impact.
- **Depth limit**: When capturing call stacks, choose a reasonable depth (e.g., 10–20) to avoid excessive overhead.
- **Source callback**: For viewing source code in the Tracy UI, register a source callback that returns the source file content.