---
name: shine-asset-registry
description: "ShineAsset registry and runtime asset system. Invoke when working with EditorAssetRegistry, RuntimeAssetRegistry, AssetDependencyGraph, AssetHandle, AssetBase, AssetMetadata (.sasset), UUID generation, or the full asset type lifecycle (define → register factory → cook → resolve)."
---

# ShineAsset — Registry & Runtime

All sources live under `src/editor/ShineAsset/`.  
- `core/` — runtime types (`AssetBase`, `AssetHandle`, `RuntimeAssetRegistry`, …)  
- `registry/` — editor-only (`EditorAssetRegistry`, `AssetDependencyGraph`, `EditorAssetRegistryIndex`)  
- `metadata/` — `.sasset` JSON schema (`AssetMetadata`, `AssetImportSettings`)

---

## File Map

| Header | Namespace | Purpose |
|--------|-----------|---------|
| `core/AssetTypes.h` | `shine::editor::asset::AssetTypeId` / `SubAssetTypeId` | `constexpr std::string_view` type IDs: `Model`, `Texture`, `Material`, `World`, … / `Mesh`, `Skeleton`, … |
| `core/AssetBase.h` | `shine::asset` | Base class for loaded assets. `EAssetState` { Unloaded, Loading, Loaded, Failed }. UUID + typeId + atomic state. |
| `core/AssetHandle.h` | `shine::asset` | `AssetHandle<T>` — UUID-based stable reference. Serializes as raw UUID via Glaze. `Resolve(registry)` → `shared_ptr<T>`. |
| `core/AssetFactory.h` | `shine::asset` | `AssetCreatorFn = std::function<shared_ptr<AssetBase>(STextView uuid)>`. |
| `core/AssetUuidHelper.h` | `shine::editor::asset` | UUID generation, parsing, validation. |
| `core/RuntimeAssetRegistry.h` | `shine::asset` | Thread-safe UUID → `shared_ptr<AssetBase>`. `RequestLoad()`, factory system. `shine::Subsystem`. |
| `registry/EditorAssetRegistry.h` | `shine::editor::asset` | UUID → disk path + metadata. Scan, register, relocate, delete, dependency queries. `shine::Subsystem`. |
| `registry/EditorAssetRegistryIndex.h` | `shine::editor::asset` | Fast-startup index `Content/.assetindex`. `SaveRegistryIndex()` / `LoadRegistryIndex()`. |
| `registry/AssetDependencyGraph.h` | `shine::editor::asset` | Bidirectional DAG with cycle detection. Forward + reverse lookups. |
| `metadata/AssetMetadata.h` | `shine::editor::asset` | `.sasset` JSON schema structs. `ReadAssetMetadataFile()` / `WriteAssetMetadataFile()`. |

---

## .sasset File Format

```jsonc
{
  "formatVersion": "2.0",
  "asset": {
    "uuid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
    "type": "model",
    "sourceFile": "Content/model/barrel/barrel.glb",
    "imported": true,
    "lastImportTime": "2025-01-15T10:30:00Z",
    "importSettings": { },           // glz::raw_json — type-specific blob
    "subAssets": [
      { "uuid": "…", "type": "mesh", "name": "Barrel_LOD0", "properties": {} }
    ],
    "nodeTree": {
      "name": "Root",
      "transform": { "translation": [0,0,0], "rotation": [0,0,0,1], "scale": [1,1,1] },
      "components": [{ "type": "mesh", "uuid": "sub-asset-uuid" }],
      "children": []
    },
    "dependencies": ["uuid-of-material", "uuid-of-texture"],
    "userData": { "tags": ["props"], "extra": {} }
  }
}
```

Read/write (serialization boundary — `std::string_view`):
```cpp
#include "editor/ShineAsset/metadata/AssetMetadata.h"

auto result  = ReadAssetMetadataFile("Content/model/barrel/barrel.sasset");
auto written = WriteAssetMetadataFile(meta, "Content/model/barrel/barrel.sasset");
```

---

## UUID Generation

```cpp
#include "editor/ShineAsset/core/AssetUuidHelper.h"
using namespace shine::editor::asset;

SString uuid  = GenerateUUIDString();    // V4 random
SString uuid7 = GenerateV7UUIDString();  // V7 time-ordered (preferred for new assets)
bool valid    = IsValidUUIDString(uuid);
```

UUID format: RFC 9562 canonical lowercase with hyphens `xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`.  
**Always prefer V7** for new assets — time-ordered for better index locality.

---

## AssetHandle

```cpp
#include "editor/ShineAsset/core/AssetHandle.h"

AssetHandle<MyMeshAsset> handle("xxxxxxxx-xxxx-…");

// Transparent Glaze serialization — reads/writes just the UUID string
struct MyComponent {
    AssetHandle<MyMeshAsset> mesh;  // JSON: "mesh": "uuid-string"
};

// Resolve to shared_ptr (nullptr if not loaded)
auto ptr = handle.Resolve(runtimeRegistry);
```

---

## RuntimeAssetRegistry

Thread-safe. Registered as `shine::Subsystem` in `EditorCompositionRoot`.

```cpp
#include "editor/ShineAsset/core/RuntimeAssetRegistry.h"
using namespace shine::asset;

auto& rr = ctx.GetSystem<RuntimeAssetRegistry>();

// Register a factory
rr.RegisterFactory("model", [](STextView uuid) {
    return std::make_shared<MyModelAsset>(uuid, "model");
});

// Request async load
auto [result, asset] = rr.RequestLoad("uuid", "model");
// result: Queued / AlreadyLoaded / AlreadyLoading / NoFactory / InvalidUUID

// Direct register / query
rr.Register(std::make_shared<MyAsset>(uuid, typeId));
auto found = rr.FindAs<MyAsset>("uuid");
```

---

## EditorAssetRegistry

Single-threaded (editor main thread only). Registered as `shine::Subsystem` in `EditorCompositionRoot`.

```cpp
#include "editor/ShineAsset/registry/EditorAssetRegistry.h"
using namespace shine::editor::asset;

auto& er = ctx.GetSystem<EditorAssetRegistry>();

// Scan content directory (fallback on startup)
er.Scan(contentRoot);

// Register from parsed metadata
er.Register(diskPath, assetRecord);

// Query
const EditorAssetEntry* entry = er.Find("uuid");
const EditorAssetEntry* entry = er.FindByPath(path);
bool known    = er.IsKnown("uuid");
bool dangling = er.IsDangling("uuid");  // .sasset deleted but still referenced

// Dependency queries
const auto& rev   = er.GetDependents("uuid");   // who depends on me?
const auto& graph = er.DependencyGraph();        // forward dependency graph

// File system events
er.OnFileMoved(oldPath, newPath);
auto affected = er.OnFileDeleted(path);           // returns dependent UUIDs

// Safe delete (checks dependents)
auto result = er.TryDelete("uuid", EDeletePolicy::SafeOnly);

// Iteration
er.ForEach([](const EditorAssetEntry& e) { /* … */ });
```

### EditorAssetEntry Fields

```cpp
struct EditorAssetEntry {
    std::string     uuid;
    std::string     diskPath;    // absolute path to .sasset
    AssetRecord     record;      // parsed from .sasset (type, sourceFile, subAssets, …)
    bool            isDangling;  // true if .sasset file is missing on disk
};
```

---

## AssetDependencyGraph

```cpp
#include "editor/ShineAsset/registry/AssetDependencyGraph.h"
using namespace shine::editor::asset;

AssetDependencyGraph graph;

graph.SetDependencies("asset-A", { "dep-B", "dep-C" });

const auto& fwd = graph.GetDependencies("asset-A");   // → {dep-B, dep-C}
const auto& rev = graph.GetDependents("dep-B");        // → {asset-A}
bool wouldCycle = graph.WouldCreateCycle("dep-B", "asset-A"); // → true

graph.RemoveAsset("asset-A");
```

---

## Creating a New Asset Type

1. Add type ID to `core/AssetTypes.h`:
   ```cpp
   namespace shine::editor::asset::AssetTypeId {
       inline constexpr std::string_view ParticleSystem = "particle_system";
   }
   ```

2. Create a concrete `AssetBase` subclass (runtime code — can live anywhere):
   ```cpp
   #include "editor/ShineAsset/core/AssetBase.h"

   class ParticleSystemAsset : public shine::asset::AssetBase {
   public:
       explicit ParticleSystemAsset(STextView uuid)
           : AssetBase(uuid, "particle_system") {}
       // runtime data members …
   };
   ```

3. Register factory in `RuntimeAssetRegistry` at startup:
   ```cpp
   rr.RegisterFactory("particle_system", [](STextView uuid) {
       return std::make_shared<ParticleSystemAsset>(uuid);
   });
   ```

4. Create an `IAssetImporter` (see **shine-asset-import** skill).

5. Optionally create an `IAssetCooker` and register with `CookingPipeline`.

6. Reference via `AssetHandle<ParticleSystemAsset>` in serializable structs.

---

## Subsystem Lifecycle

```
EditorCompositionRoot::RegisterSystems(ctx)
    ├── ctx.Register(new RuntimeAssetRegistry())
    ├── ctx.Register(new EditorAssetRegistry())
    └── ctx.Register(new ImportPipeline())       ← see shine-asset-import skill

EditorAssetRegistry::Init(ctx)
    ├── Try LoadRegistryIndex("Content/.assetindex")   ← fast startup
    ├── Fallback: Scan("Content/")
    └── Subscribe FileWatchService::OnFileChanged → OnFileChangeEvent()

EditorAssetRegistry::Shutdown(ctx)
    ├── SaveRegistryIndex("Content/.assetindex")
    └── Unsubscribe file watcher
```

---

## Conventions

- `AssetMetadata` structs use `std::string` — Glaze serialization boundary. Internal code uses `SString` / `STextView`.
- `RuntimeAssetRegistry` is mutex-protected. `EditorAssetRegistry` is main-thread only.
- **Dangling**: entry where `.sasset` is deleted but other assets still hold its UUID. `isDangling = true`.
- Build: `bcrypt.lib` linked to `MainEngine` for UUID generation (not to `ShineAsset` static lib itself).
