---
name: shine-asset-import
description: "ShineAsset import pipeline. Invoke when implementing or using IAssetImporter, ImportPipeline, REGISTER_IMPORTER macro, AssetImportContext, AssetImportSettings, or adding support for new file formats (GLTF, OBJ, texture, FBX, …)."
---

# ShineAsset — Import Pipeline

All import sources live under `src/editor/ShineAsset/importers/`.  
EDITOR-ONLY — never include from runtime code.

---

## File Map

| Header | Purpose |
|--------|---------|
| `IAssetImporter.h` | Abstract base class + `AssetImportContext` + `ImportResult`. |
| `AssetImportSettings.h` | `AssetImportSettings` base + `SerializeImportSettings<T>()` / `ParseImportSettings<T>()`. |
| `ImporterAutoRegistry.h` | Self-registration singleton + `REGISTER_IMPORTER(ClassName)` macro. |
| `ImportPipeline.h` | Subsystem — holds all importers, `FindImporter()`, `ExecuteImport()`. |
| `GltfAssetImporter.h` | Concrete importer for `.gltf` / `.glb`. |
| `ObjAssetImporter.h` | Concrete importer for `.obj`. Settings: `ObjImportSettings { scale, flipUV }`. |
| `TextureAssetImporter.h` | Concrete importer for `.png` / `.jpg` / `.jpeg`. Settings: `TextureImportSettings { generateMipmaps, sRGB }`. |

---

## IAssetImporter Interface

```cpp
#include "editor/ShineAsset/importers/IAssetImporter.h"

class IAssetImporter {
public:
    virtual std::string_view GetName() const noexcept = 0;
    virtual std::vector<std::string_view> SupportedExtensions() const noexcept = 0;

    // Default: extension match against SupportedExtensions().
    // Override for magic-byte / content-based detection.
    virtual bool CanImport(const std::filesystem::path& sourceFile) const noexcept;

    // Called on a worker thread — must be thread-safe.
    virtual ImportResult Import(const AssetImportContext& ctx) = 0;

    // Render ImGui widgets for type-specific settings each frame the popup is open.
    // Parse inOutSettings with ParseImportSettings<T>(), render, re-serialize on change.
    // Return true if settings changed. Default: no-op, returns false.
    virtual bool RenderImportSettingsUI(glz::raw_json& inOutSettings);
};
```

### AssetImportContext Fields

| Field | Type | Description |
|-------|------|-------------|
| `sourceFile` | `std::filesystem::path` | Absolute path to the source file. |
| `contentRoot` | `std::filesystem::path` | Absolute path to `Content/`. |
| `outputSAssetPath` | `std::filesystem::path` | Destination `.sasset` path (pipeline-chosen). |
| `rootUUID` | `std::string` | Pre-generated V7 UUID for the root asset. |
| `savedImportSettings` | `glz::raw_json` | Previously saved settings blob (empty on first import). |
| `onProgress` | `function<void(string_view, float)>` | Optional progress callback `(message, 0..1)`. |

### ImportResult Fields

```cpp
struct ImportResult {
    bool          succeeded = false;
    AssetMetadata metadata;       // valid only when succeeded == true
    std::string   errorMessage;   // non-empty on failure
};
```

---

## ImportPipeline (Subsystem)

Registered in `EditorCompositionRoot`. `Init()` calls `ImporterAutoRegistry::CreateAll()` to instantiate every `REGISTER_IMPORTER` class.

```cpp
#include "editor/ShineAsset/importers/ImportPipeline.h"
using namespace shine::editor::asset;

auto& pipeline = ctx.GetSystem<ImportPipeline>();

// Resolve importer by extension (CanImport())
IAssetImporter* imp = pipeline.FindImporter(sourceFile); // nullptr if unsupported

// Full import sequence:
//   Import() → create_directories(destDir/<stem>/) → write .sasset → register in registry
ImportResult res = pipeline.ExecuteImport(
    *imp,
    sourceFile,      // absolute path to source file
    destDir,         // directory to write output into
    contentRoot,     // Content/ root
    savedSettings,   // glz::raw_json — pass {} on first import
    &editorRegistry  // EditorAssetRegistry* — pass nullptr to skip registration
);
```

**Output layout**: `destDir/<stem>/<stem>.sasset` — each imported asset gets its own named subfolder.

### Manual importer registration (plugins / after Init)

```cpp
pipeline.RegisterImporter(std::make_shared<MyImporter>());
```

---

## REGISTER_IMPORTER — Self-Registration Macro

Place exactly once in the importer's `.cpp`. No other file needs modification.

```cpp
// In MyImporter.cpp:
#include "ImporterAutoRegistry.h"
REGISTER_IMPORTER(shine::editor::asset::MyImporter)
```

**How it works**: defines a file-scope `ImporterRegistrar<T>` whose constructor enqueues a factory lambda in `ImporterAutoRegistry` before `main()`. `ImportPipeline::Init()` calls `ImporterAutoRegistry::CreateAll()` to instantiate all registered importers. Safe because all `.cpp` files in the `importers/` directory are compiled into the same static library target, so the linker always includes every object file.

---

## Import Settings

```cpp
#include "editor/ShineAsset/importers/AssetImportSettings.h"

// Define type-specific settings
struct MyImportSettings : AssetImportSettings {
    float scale      = 1.0f;
    bool  flipUV     = false;
    std::string lod  = "auto";
};

// Inside Import() or RenderImportSettingsUI():
auto settings = ParseImportSettings<MyImportSettings>(ctx.savedImportSettings);
// ... modify settings via imgui ...
auto blob = SerializeImportSettings<MyImportSettings>(settings);
```

Settings are stored in `.sasset` as `importSettings` (raw JSON blob via Glaze).  
`AssetImportSettings` base struct uses `std::string` (serialization boundary — Glaze requirement).

---

## Creating a New Importer (Step-by-Step)

1. Create `src/editor/ShineAsset/importers/FbxAssetImporter.h`:

```cpp
#pragma once
#include "IAssetImporter.h"
#include "AssetImportSettings.h"

namespace shine::editor::asset {

struct FbxImportSettings : AssetImportSettings {
    float scale = 1.0f;
};

class FbxAssetImporter final : public IAssetImporter {
public:
    [[nodiscard]] std::string_view GetName() const noexcept override;
    [[nodiscard]] std::vector<std::string_view> SupportedExtensions() const noexcept override;
    [[nodiscard]] ImportResult Import(const AssetImportContext& ctx) override;
    bool RenderImportSettingsUI(glz::raw_json& inOutSettings) override;
};

} // namespace shine::editor::asset
```

2. Create `src/editor/ShineAsset/importers/FbxAssetImporter.cpp`:

```cpp
#include "FbxAssetImporter.h"
#include "ImporterAutoRegistry.h"
#include "editor/ShineAsset/core/AssetUuidHelper.h"
#include "editor/ShineAsset/metadata/AssetMetadata.h"

REGISTER_IMPORTER(shine::editor::asset::FbxAssetImporter)

namespace shine::editor::asset {

std::string_view FbxAssetImporter::GetName() const noexcept { return "FBX Importer"; }

std::vector<std::string_view> FbxAssetImporter::SupportedExtensions() const noexcept {
    return { ".fbx" };
}

ImportResult FbxAssetImporter::Import(const AssetImportContext& ctx) {
    auto settings = ParseImportSettings<FbxImportSettings>(ctx.savedImportSettings);

    ImportResult result;
    // 1. Load ctx.sourceFile with your FBX parser
    // 2. Build AssetMetadata:
    //      meta.asset.uuid       = ctx.rootUUID
    //      meta.asset.type       = "model"
    //      meta.asset.sourceFile = relative path from contentRoot
    //      meta.asset.subAssets  = { { GenerateV7UUIDString(), "mesh", "MeshName", {} }, … }
    // 3. Write .sasset — the pipeline handles this after Import() returns
    result.succeeded = true;
    result.metadata  = std::move(meta);
    return result;
}

bool FbxAssetImporter::RenderImportSettingsUI(glz::raw_json& inOutSettings) {
    auto s = ParseImportSettings<FbxImportSettings>(inOutSettings);
    bool changed = false;
    changed |= ImGui::SliderFloat("Scale", &s.scale, 0.001f, 100.0f);
    if (changed) inOutSettings = SerializeImportSettings<FbxImportSettings>(s);
    return changed;
}

} // namespace shine::editor::asset
```

3. No CMake changes needed — `dirs` glob in `ShineAsset.json` auto-discovers new files.

---

## Existing Importers

| Class | Extensions | Settings struct |
|-------|-----------|----------------|
| `GltfAssetImporter` | `.gltf`, `.glb` | — |
| `ObjAssetImporter` | `.obj` | `ObjImportSettings { float scale=1; bool flipUV=false; }` |
| `TextureAssetImporter` | `.png`, `.jpg`, `.jpeg` | `TextureImportSettings { bool generateMipmaps=true; bool sRGB=true; }` |

---

## Conventions

- Import context and result use `std::string` (serialization boundary, not `SString`).
- Generate sub-asset UUIDs via `GenerateV7UUIDString()` (time-ordered).
- `Import()` is called on a worker thread — must be thread-safe.
- `RenderImportSettingsUI()` is called on the main thread each frame the popup is open.
