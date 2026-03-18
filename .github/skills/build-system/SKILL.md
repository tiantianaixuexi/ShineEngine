---
name: build-system
description: "ShineEngine build system (Build.bat + CMake module JSON). Invoke when building targets, adding new modules, configuring compiler/platform flags, running tests or executables, switching editor/runtime mode, or troubleshooting build failures."
---

# ShineEngine Build System

## Invoke When

- Building the engine, a module, an executable, or a test
- Adding a new module (creating its JSON registration)
- Switching between Editor / Runtime build modes
- Selecting a compiler (MSVC / Clang / GCC)
- Troubleshooting CMake configuration or link errors
- Running the engine or a test after build

---

## Quick Reference

All commands run from the project root (`E:\c++\ShineEngine`).

### Build & Run MainEngine

```powershell
.\build.bat run                   # Debug, build + run
.\build.bat run --release         # Release, build + run
.\build.bat x64                   # Debug, build + run (alternative)
.\build.bat release               # Release, build only (prompts to run)
.\build.bat run --bt              # MSVC: keep normal output, write readable /Bt+ summary to Logs/
```

### Build a Named Executable

```powershell
.\build.bat exe EngineLauncher          # Build only
.\build.bat exe EngineLauncher --run    # Build + run
```

### Build a Module (library)

```powershell
.\build.bat module MyModule             # Debug
.\build.bat module MyModule --release   # Release
```

### Build & Run Tests

```powershell
.\build.bat test                        # Build + run default TestRunner (Debug)
.\build.bat test SimplePerfTest         # Build + run named test
.\build.bat test SimplePerfTest --release
```

### Build WASM Targets

```powershell
.\build.bat wasm                        # default target: smallwasm
.\build.bat wasm MyWasmTarget --release
```

### Utility

```powershell
.\build.bat clean                 # Delete build dir + exe artifacts
.\build.bat list                  # List all Module/*.json files
.\build.bat compile_commands      # Generate compile_commands.json for clangd
```

---

## Compiler Selection

Default is MSVC (Visual Studio 2026). Override with flags:

```powershell
.\build.bat run --clang           # Clang + Ninja (build_clang/)
.\build.bat run --gcc             # GCC   + Ninja (build_gcc/)
.\build.bat run --msvc            # MSVC (default, build_msvc/)
.\build.bat run --compiler clang  # same as --clang
```

Each compiler uses a separate build directory (`build_msvc/`, `build_clang/`, `build_gcc/`).

---

## Global Flags

| Flag | Effect |
|------|--------|
| `--release` | Use Release configuration (default is Debug) |
| `--run` | Run executable after building (for `exe` command) |
| `--clean-first` | Pass `--clean-first` to `cmake --build` |
| `--no-pause` | Do not pause at script end (CI-friendly, on by default) |
| `--editor` | Enable editor mode — defines `BUILD_EDITOR` macro (default) |
| `--no-editor` / `--runtime` | Disable editor — excludes editor-only modules |
| `--enable-module` | Enable C++20 modules (`SHINE_BUILD_MODULE`) |
| `--bt` | MSVC only: enable `/Bt+` per-file compile timing and write a readable summary file to `Logs/` |

---

## Module JSON Format

Modules are registered via JSON files in `Module/` (libraries) or `Module/Program/` / `dev/Program/` (executables).

```
Module/
  math.json               ← library module
  texture.json
  editor/
    ShineAsset.json        ← editor-only library
  test/
    SimplePerfTest.json    ← test executable
  Program/
    EngineLauncher.json    ← standalone program
```

### Minimal Library Module

```json
{
  "name": "my_module",
  "files": [
    "src/my_module/my_module.cpp",
    "src/my_module/my_module.h"
  ],
  "deps": ["fmt"],
  "type": ["lib"]
}
```

### Executable Module

```json
{
  "name": "MyApp",
  "files": ["dev/Program/MyApp/main.cpp"],
  "deps": ["fmt", "math"],
  "type": ["exe"],
  "platform": ["Windows"],
  "buildMode": ["both"]
}
```

### Key Fields

| Field | Type | Description |
|-------|------|-------------|
| `name` | string | Target name (defaults to filename without `.json`) |
| `files` | string[] | Source/header paths relative to project root |
| `dirs` | string[] | Directories to glob for sources automatically |
| `deps` | string[] | Other module names to link against |
| `defines` | string[] | Compile definitions |
| `include_dirs` | string[] | Additional include directories |
| `type` | string or string[] | `"lib"`, `"static"`, `"shared"`, `"exe"`, `"third"`, `"interface"`, `"subcmake"` |
| `buildMode` | string or string[] | `"editor"`, `"runtime"`, or `"both"` (default: `"both"`) |
| `platform` | string[] | Platform tags, e.g. `["Windows"]` |
| `link.debug.lib` | string[] | Debug-config third-party `.lib` files |
| `link.release.lib` | string[] | Release-config third-party `.lib` files |
| `files_module` | string[] | Files used only when C++20 modules are enabled |
| `files_header` | string[] | Files used only when C++20 modules are disabled |
| `files_compile` | string[] | Whitelist of files to compile (rest become header-only) |

### Module Types

| Type | CMake Target | Notes |
|------|-------------|-------|
| `lib` | `add_library(STATIC)` | Default. Static library |
| `static` | `add_library(STATIC)` | Explicit static library |
| `shared` | `add_library(SHARED)` | Shared / DLL |
| `exe` | `add_executable` | Standalone executable |
| `third` | Custom target | Third-party code with special handling |
| `interface` | `add_library(INTERFACE)` | Header-only interface library |
| `subcmake` | `add_custom_target` | External CMake sub-project |

---

## Build Modes (Editor vs Runtime)

Set `"buildMode"` in module JSON to control when a module is included:

- `"editor"` — only when `--editor` (default)
- `"runtime"` — only when `--runtime` / `--no-editor`
- `"both"` — always included (default if omitted)

The build flag `--editor` adds the `BUILD_EDITOR` compile definition globally. Code can use:

```cpp
#ifdef BUILD_EDITOR
// editor-only logic
#endif
```

---

## Output Locations

| Artifact | Path |
|----------|------|
| MainEngine (Debug) | `exe/MainEngined.exe` |
| MainEngine (Release) | `exe/MainEngine.exe` |
| Other executables | `<build_dir>/exe/<Name>.exe` or `exe/<Name>.exe` |
| Static libraries | `<build_dir>/build/` |
| DLLs | `<build_dir>/exe/` |
| compile_commands.json | project root (copied from build dir) |

---

## Troubleshooting

| Symptom | Likely Cause |
|---------|-------------|
| `CMake configuration failed` | Missing CMake or wrong generator; run `cmake --version` |
| `Clang/GCC not found` | Compiler not in PATH |
| `Build failed` on a module | Check `Module/<name>.json` for typos in `files` or `deps` |
| Module not built | `buildMode` doesn't match current `--editor`/`--runtime` flag |
| Link errors | Missing entry in `deps` or `link.debug/release.lib` |
