---
name: shine-asset-system
description: "ShineAsset system architecture, API reference, and usage patterns. Invoke when creating/modifying/querying asset types, importers, cookers, metadata (.sasset), dependency graphs, or editor/runtime asset registries. Covers the full asset pipeline: import → register → cook → runtime resolve."
---

# ShineAsset System

Module: `Module/editor/ShineAsset.json` (static lib, editor-only).  
All headers/sources live under `src/editor/ShineAsset/`.  
Deps: `glaze`, `gltf_loader`, `engine_log`, `fmt`.  
System lib: `bcrypt.lib` (linked to MainEngine for UUID generation).

---

## Architecture Overview

```
Source File (.gltf/.obj/…)
    │  IAssetImporter::Import()
    ▼
.sasset (JSON metadata on disk)
    │  EditorAssetRegistry::Register()         ← editor only
    ▼
EditorAssetRegistry  ──→  AssetDependencyGraph
    │  IAssetCooker::Cook()
    ▼
Cooked Binary (.bin)
    │  RuntimeAssetRegistry::RequestLoad()
    ▼
RuntimeAssetRegistry  ──→  AssetBase (shared_ptr, loaded)
    │  AssetHandle<T>::Resolve()
    ▼
Gameplay / Renderer usage
```

Two-registry pattern:
- **EditorAssetRegistry** — editor-only, UUID ↔ disk path + metadata, dependency graph, file watcher integration. Subsystem registered in `EditorCompositionRoot`.
- **RuntimeAssetRegistry** — shipping + editor, UUID → loaded `AssetBase`, thread-safe. Subsystem registered in `EditorCompositionRoot`.

---

## File Map

| Header | Namespace | Purpose |
|--------|-----------|---------|
| `AssetTypes.h` | `shine::editor::asset::AssetTypeId` / `SubAssetTypeId` | `constexpr string_view` type IDs: `Model`, `Texture`, `Material`, `World`, … / `Mesh`, `Skeleton`, … |
| `AssetBase.h` | `shine::asset` | Base class for loaded assets. `EAssetState` { Unloaded, Loading, Loaded, Failed }. UUID + typeId + atomic state. |
| `AssetHandle.h` | `shine::asset` | `AssetHandle<T>` — UUID-based stable reference. Serializes as raw UUID via Glaze. `Resolve(registry)` → `shared_ptr<T>`. |
| `AssetMetadata.h` | `shine::editor::asset` | `.sasset` JSON schema structs (serialization boundary — uses `std::string`). |
| `AssetUuidHelper.h` | `shine::editor::asset` | UUID generation (`GenerateUUIDString()`, `GenerateV7UUIDString()`), parsing, validation. |
| `AssetFactory.h` | `shine::asset` | `AssetCreatorFn = std::function<shared_ptr<AssetBase>(STextView uuid)>` |
| `AssetImportSettings.h` | `shine::editor::asset` | Extensible typed import settings with `SerializeImportSettings<T>()` / `ParseImportSettings<T>()`. |
| `IAssetImporter.h` | `shine::editor::asset` | Abstract importer interface (`Import(ctx) → ImportResult`). |
| `IAssetCooker.h` | `shine::editor::asset` | Abstract cooker interface (`Cook(ctx) → CookResult`). |
| `RuntimeAssetRegistry.h` | `shine::asset` | Thread-safe UUID → `shared_ptr<AssetBase>` map. `RequestLoad()`, factory system. Inherits `shine::Subsystem`. |
| `EditorAssetRegistry.h` | `shine::editor::asset` | UUID → disk path + metadata. Scan, register, relocate, delete, dependency queries. Inherits `shine::Subsystem`. |
| `EditorAssetRegistryIndex.h` | `shine::editor::asset` | Fast-startup index (`Content/.assetindex`). `SaveRegistryIndex()` / `LoadRegistryIndex()`. |
| `AssetDependencyGraph.h` | `shine::editor::asset` | Bidirectional DAG with cycle detection. Forward + reverse lookups. |
| `GltfAssetImporter.h` | `shine::editor::asset` | Concrete `IAssetImporter` for `.gltf/.glb/.obj`. |
| `StaticMeshCooker.h` | `shine::editor::asset` | Concrete `IAssetCooker` for `Model` type → binary mesh blob. |
| `CookingPipeline.h` | `shine::editor::asset` | `RegisterCooker()`, `CookAll()`, `CookSingle()`. |

---

## .sasset File Format (JSON)

```jsonc
{
  "formatVersion": "2.0",
  "asset": {
    "uuid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
    "type": "model",              // AssetTypeId constant
    "sourceFile": "Content/model/barrel.glb",
    "imported": true,
    "lastImportTime": "2025-01-15T10:30:00Z",
    "importSettings": { ... },    // glz::raw_json — type-specific
    "subAssets": [
      { "uuid": "…", "type": "mesh", "name": "Barrel_LOD0", "properties": {} }
    ],
    "nodeTree": {                 // optional scene hierarchy
      "name": "Root",
      "transform": { "translation": [0,0,0], "rotation": [0,0,0,1], "scale": [1,1,1] },
      "components": [{ "type": "mesh", "uuid": "sub-asset-uuid" }],
      "children": []
    },
    "dependencies": ["uuid-of-material", "uuid-of-texture"],
    "userData": { "tags": ["props", "barrel"], "extra": {} }
  }
}
```

Read/write helpers (serialization boundary — accepts `std::string_view`):
```cpp
#include "editor/ShineAsset/AssetMetadata.h"

auto result = ReadAssetMetadataFile("Content/model/barrel.sasset");
auto written = WriteAssetMetadataFile(meta, "Content/model/barrel.sasset");
```

---

## Key APIs

### UUID Generation

```cpp
#include "editor/ShineAsset/AssetUuidHelper.h"
using namespace shine::editor::asset;

SString uuid  = GenerateUUIDString();    // V4 random
SString uuid7 = GenerateV7UUIDString();  // V7 time-ordered (preferred for new assets)
bool valid    = IsValidUUIDString(uuid);
```

### AssetHandle (reference an asset by UUID)

```cpp
#include "editor/ShineAsset/AssetHandle.h"

// Construct from UUID string
AssetHandle<MyMeshAsset> handle("xxxxxxxx-xxxx-…");

// Serialize transparently — Glaze reads/writes just the UUID string
struct MyComponent {
    AssetHandle<MyMeshAsset> mesh;  // JSON: "mesh": "uuid-string"
};

// Resolve to loaded object
auto ptr = handle.Resolve(runtimeRegistry);  // shared_ptr<MyMeshAsset> or nullptr
```

### RuntimeAssetRegistry

```cpp
#include "editor/ShineAsset/RuntimeAssetRegistry.h"
using namespace shine::asset;

auto& rr = ctx.GetSystem<RuntimeAssetRegistry>();

// Register a factory
rr.RegisterFactory("model", [](STextView uuid) {
    return std::make_shared<MyModelAsset>(uuid, "model");
});

// Request async load
auto [result, asset] = rr.RequestLoad("uuid", "model");
// result: Queued / AlreadyLoaded / AlreadyLoading / NoFactory / InvalidUUID

// Direct register/query
rr.Register(std::make_shared<MyAsset>(uuid, typeId));
auto found = rr.FindAs<MyAsset>("uuid");
```

### EditorAssetRegistry

```cpp
#include "editor/ShineAsset/EditorAssetRegistry.h"
using namespace shine::editor::asset;

auto& er = ctx.GetSystem<EditorAssetRegistry>();

// Scan content directory
er.Scan(contentRoot);

// Register from metadata
er.Register(diskPath, record);

// Query
const EditorAssetEntry* entry = er.Find("uuid");
const EditorAssetEntry* entry = er.FindByPath(path);
bool known    = er.IsKnown("uuid");
bool dangling = er.IsDangling("uuid");

// Dependency queries
const auto& deps = er.GetDependents("uuid");   // reverse: who depends on me?
const auto& graph = er.DependencyGraph();       // forward: who do I depend on?

// Relocation / deletion
er.OnFileMoved(oldPath, newPath);
auto affected = er.OnFileDeleted(path);         // returns dependent UUIDs
auto result   = er.TryDelete("uuid", EDeletePolicy::SafeOnly);

// Iteration
er.ForEach([](const EditorAssetEntry& e) {
    // process each entry
});
```

### AssetDependencyGraph

```cpp
#include "editor/ShineAsset/AssetDependencyGraph.h"
using namespace shine::editor::asset;

AssetDependencyGraph graph;

// Set forward dependencies (replaces existing)
graph.SetDependencies("asset-A", { "dep-B", "dep-C" });

// Query
const auto& fwd = graph.GetDependencies("asset-A");  // → {dep-B, dep-C}
const auto& rev = graph.GetDependents("dep-B");       // → {asset-A}
bool wouldCycle = graph.WouldCreateCycle("dep-B", "asset-A");  // → true

// Clean up
graph.RemoveAsset("asset-A");
```

---

## Creating a New Importer

1. Create header and source under `src/editor/ShineAsset/`.
2. Inherit from `IAssetImporter`:

```cpp
#include "editor/ShineAsset/IAssetImporter.h"
#include "editor/ShineAsset/AssetUuidHelper.h"

namespace shine::editor::asset {

class FbxAssetImporter final : public IAssetImporter {
public:
    [[nodiscard]] std::string_view GetName() const noexcept override { return "FBX Importer"; }
    [[nodiscard]] std::vector<std::string_view> SupportedExtensions() const noexcept override {
        return { ".fbx" };
    }
    [[nodiscard]] ImportResult Import(const AssetImportContext& ctx) override {
        ImportResult result;
        // 1. Load source file from ctx.sourceFile
        // 2. Build AssetMetadata (uuid = ctx.rootUUID, type, subAssets, nodeTree)
        // 3. Generate sub-asset UUIDs via GenerateV7UUIDString()
        // 4. Write .sasset via WriteAssetMetadataFile(meta, ctx.outputSAssetPath)
        result.succeeded = true;
        result.metadata = std::move(meta);
        return result;
    }
};

} // namespace shine::editor::asset
```

3. The new `.cpp` is auto-discovered by the `dirs` glob in `ShineAsset.json`.

---

## Creating a New Cooker

1. Create header and source under `src/editor/ShineAsset/`.
2. Inherit from `IAssetCooker`:

```cpp
#include "editor/ShineAsset/IAssetCooker.h"

namespace shine::editor::asset {

class TextureCooker final : public IAssetCooker {
public:
    [[nodiscard]] std::string_view GetName() const noexcept override { return "Texture Cooker"; }
    [[nodiscard]] std::vector<std::string_view> SupportedTypeIds() const noexcept override {
        return { AssetTypeId::Texture };
    }
    [[nodiscard]] CookResult Cook(const AssetCookContext& ctx) override {
        CookResult result;
        // 1. Read source from ctx.metadata.asset.sourceFile
        // 2. Compress / transcode per ctx.platform
        // 3. Write binary to ctx.outputDir
        // 4. Populate result.outputFiles
        result.succeeded = true;
        return result;
    }
};

} // namespace shine::editor::asset
```

3. Register in CookingPipeline: `pipeline.RegisterCooker(std::make_shared<TextureCooker>())`.

---

## Creating a New Asset Type

1. Add type ID constant to `AssetTypes.h`:
   ```cpp
   inline constexpr std::string_view ParticleSystem = "particle_system";
   ```
2. Create a concrete `AssetBase` subclass:
   ```cpp
   class ParticleSystemAsset : public shine::asset::AssetBase {
   public:
       explicit ParticleSystemAsset(STextView uuid)
           : AssetBase(uuid, "particle_system") {}
       // … runtime data members
   };
   ```
3. Register factory in `RuntimeAssetRegistry`:
   ```cpp
   rr.RegisterFactory("particle_system", [](STextView uuid) {
       return std::make_shared<ParticleSystemAsset>(uuid);
   });
   ```
4. Create an `IAssetImporter` for the source format.
5. Optionally create an `IAssetCooker` for cooking.
6. Use `AssetHandle<ParticleSystemAsset>` for serializable references.

---

## Import Settings Extension

```cpp
#include "editor/ShineAsset/AssetImportSettings.h"

struct TextureImportSettings : AssetImportSettings {
    int maxResolution = 4096;
    bool generateMips = true;
    std::string format = "BC7";
};

// Serialize into raw JSON blob for storage in AssetRecord::importSettings
auto raw = SerializeImportSettings<TextureImportSettings>(settings);

// Parse back from raw blob inside importer
auto parsed = ParseImportSettings<TextureImportSettings>(record.importSettings);
```

---

## Editor Integration Points

| Component | File | How it connects |
|-----------|------|-----------------|
| `EditorCompositionRoot` | `src/editor/main_editor/EditorCompositionRoot.cpp` | Registers `RuntimeAssetRegistry` and `EditorAssetRegistry` as subsystems via `context.Register()`. |
| `AssetsBrower` | `src/editor/browers/AssetsBrower.h` | Gets `EditorAssetRegistry*` from `EngineContext` in `onInit()`. Calls `SyncAssetRecordMove/Delete` on file operations. |
| `AssetDependencyView` | `src/editor/views/AssetDependencyView.h` | Gets `EditorAssetRegistry*`, shows forward/reverse deps for selected UUID. |
| `FileWatchService` | `src/util/watcher/FileWatchService.h` | `EditorAssetRegistry::Init()` subscribes to `OnFileChanged` for hot-reload of `.sasset` files. |
| `WorldService` | `src/gameplay/world/world_service.h` | `saveMapAsset()` / `loadMapAsset()` — writes `.sasset` for world maps, registers with `EditorAssetRegistry`. |

---

## Subsystem Lifecycle

```
EditorCompositionRoot::RegisterSystems(ctx)
    ├── ctx.Register(new RuntimeAssetRegistry())
    └── ctx.Register(new EditorAssetRegistry())

EditorAssetRegistry::Init(ctx)
    ├── Try LoadRegistryIndex("Content/.assetindex") for fast startup
    ├── Fallback: Scan("Content/")
    └── Connect FileWatchService::OnFileChanged → OnFileChangeEvent()

EditorAssetRegistry::Shutdown(ctx)
    ├── SaveRegistryIndex("Content/.assetindex")
    └── Disconnect file watcher
```

---

## Build & Test

```powershell
# Build main engine (includes ShineAsset as linked static lib)
.\build.bat run --release --msvc

# Build and run asset tests
.\build.bat test ShineAssetTest
.\build.bat test ShineAssetTest --release
```

Test file: `dev/test/ShineAssetTest/ShineAssetTest.cpp`  
Module: `Module/test/ShineAssetTest.json` (deps: `ShineAsset`, `fmt`, `glaze`)

---

## Important Conventions

- **String types**: Internal code uses `SString` / `STextView`. `AssetMetadata.h` structs use `std::string` because they are a Glaze serialization boundary.
- **UUID format**: RFC 9562 canonical lowercase with hyphens: `xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`.
- **Prefer V7 UUIDs** (`GenerateV7UUIDString()`) for new assets — time-ordered for better index locality.
- **Thread safety**: `RuntimeAssetRegistry` is mutex-protected. `EditorAssetRegistry` is single-threaded (editor main thread only).
- **Dangling**: An entry where the `.sasset` file is deleted but other assets still reference the UUID. `isDangling = true`.
- **New source files** under `src/editor/ShineAsset/` are **auto-discovered** by the `dirs` glob in the module JSON — no manual file list needed.
