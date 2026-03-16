# ShineAsset System — Architecture

## Overview

The ShineAsset system provides a unified asset lifecycle for the Shine Engine.
It is split into two registries with clearly separated responsibilities:

| Registry                | Layer        | Purpose |
|-------------------------|--------------|---------|
| `RuntimeAssetRegistry`  | Runtime      | In-memory UUID → `AssetBase` map; manages loaded asset lifetimes |
| `EditorAssetRegistry`   | Editor-only  | UUID → disk path + metadata map; discovery, relocation, deletion |

Both registries are registered as subsystems in `EngineContext` via
`EditorCompositionRoot::RegisterEditorSystems()`.

---

## .sasset File Format (v2.0)

Every asset on disk is represented by a `.sasset` JSON file.
This file contains editor metadata only — the runtime binary is produced
by the **cooking pipeline** (see below).

```json
{
  "formatVersion": "2.0",
  "asset": {
    "uuid": "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx",
    "type": "model",
    "sourceFile": "Content/Models/Character.glb",
    "imported": true,
    "lastImportTime": "2026-03-13T12:00:00Z",
    "importSettings": { ... },
    "subAssets": [
      {
        "uuid": "...",
        "type": "mesh",
        "name": "MainMesh",
        "properties": { "vertexCount": 5000 }
      }
    ],
    "nodeTree": { ... },
    "dependencies": ["uuid-of-material", "uuid-of-texture"],
    "userData": { "tags": ["character"], "extra": {} }
  }
}
```

Key design points:
- `importSettings` and `SubAssetEntry::properties` are opaque `raw_json` blobs.
  Concrete types parse them via `ParseImportSettings<T>()`.
- Adding a new asset type never requires editing `AssetMetadata.h`.
- Unknown JSON keys are ignored on read (forward compatibility).

---

## Runtime Asset System

### AssetBase

Base class for all loaded asset objects. Provides:
- Immutable UUID and type ID (set at construction)
- Atomic load state (`Unloaded → Loading → Loaded | Failed`)

### AssetHandle\<T\>

Type-safe, UUID-based reference. Serializable as a plain UUID string via Glaze.
Resolution goes through `RuntimeAssetRegistry::FindAs<T>()`.

### RuntimeAssetRegistry

Thread-safe registry (mutex-protected). Provides:
- `RegisterFactory(typeId, creator)` — per-type creator for `RequestLoad()`
- `Register(asset)` / `Unregister(uuid)` — direct asset management
- `RequestLoad(uuid, typeId)` — creates a placeholder in `Loading` state;
  the actual load is dispatched by the caller (loader subsystem)
- `Find(uuid)` / `FindAs<T>(uuid)` — lookup

---

## Editor Asset System

### EditorAssetRegistry

Subsystem that manages the mapping from UUID to on-disk `.sasset` path.
On startup it:
1. Tries to load a fast-startup index (`.assetindex`)
2. Falls back to a full `Scan()` of the Content directory
3. Connects to `FileWatchService` for hot-reload

Key operations:
- `Scan(contentRoot)` — recursive discovery of `.sasset` files
- `Register(diskPath, record)` — add or update an entry
- `OnFileMoved()` / `OnFileDeleted()` — relocation tracking
- `TryDelete(uuid, policy)` — safe or forced deletion with dependency check

On shutdown, the registry saves the fast-startup index.

### AssetDependencyGraph

Bidirectional directed graph tracking asset dependencies:
- Forward: "asset A depends on B and C"
- Reverse: "B is depended on by A"
- Cycle detection via iterative DFS (`WouldCreateCycle()`)

Updated automatically whenever metadata is registered.

### EditorAssetRegistryIndex

Persistence layer for fast editor startup. Saves/loads a compact JSON index
(`Content/.assetindex`) containing UUID, path, type, dependencies, and
last-modified timestamp. Stale entries are re-scanned on load.

---

## Import Pipeline

### IAssetImporter

Abstract interface: source file → `.sasset` metadata.

| Method | Purpose |
|--------|---------|
| `GetName()` | Human-readable name |
| `SupportedExtensions()` | File extensions this importer handles |
| `CanImport(path)` | Extension-based check (overridable for magic-byte sniffing) |
| `Import(ctx)` | Perform the import; returns `ImportResult` with metadata |

### GltfAssetImporter

Concrete importer wrapping `gltfLoader`. Supports `.gltf`, `.glb`, `.obj`.
Produces `AssetMetadata` with sub-assets for each mesh extracted.

### AssetImportSettings

Zero-coupling extensibility point. Derive from `AssetImportSettings`,
then use `SerializeImportSettings<T>()` and `ParseImportSettings<T>()`
to round-trip typed settings through the opaque `raw_json` blob.

---

## Cooking Pipeline

### IAssetCooker

Abstract interface: `.sasset` metadata → platform-specific binary.

Separation from `IAssetImporter` is intentional:
- **Import** converts source files to `.sasset` (human-readable metadata)
- **Cook** converts `.sasset` to `.bin` (compact runtime binary)

This allows re-cooking without re-importing and vice-versa.

### CookingPipeline

Orchestrator that:
1. Iterates `EditorAssetRegistry` entries
2. Finds the registered `IAssetCooker` for each asset type
3. Dispatches `Cook()` with per-platform context

### StaticMeshCooker

Concrete cooker for model assets. Outputs a compact binary mesh blob
per sub-asset: `[vertexCount:u32][indexCount:u32][vertices][normals][texcoords][indices]`.

### ECookPlatform

Enum for target platform dispatch:
`Windows_x64`, `Linux_x64`, `Android_ARM64`, `iOS_ARM64`, `WebAssembly`.

---

## Hot Reload

`EditorAssetRegistry` subscribes to `FileWatchService::OnFileChanged`.
When a `.sasset` file is modified or added:
1. The metadata is re-read and re-registered
2. The dependency graph is updated

When a `.sasset` file is deleted:
1. The entry is marked as dangling
2. Dependents can detect the broken reference

---

## Asset Browser Integration

`AssetsBrower` is injected with `EditorAssetRegistry*` at startup.
- `SyncAssetRecordMove()` → `EditorAssetRegistry::OnFileMoved()`
- `SyncAssetRecordDelete()` → `EditorAssetRegistry::OnFileDeleted()`
- `OpenEntry()` dispatches by asset type (world, material, etc.)
- Delete confirmation shows a warning when the asset has dependents

---

## World Asset Persistence

`WorldService` can save/load maps as `.sasset` files:
- `saveMapAsset(path)` — writes metadata with world settings, registers with `EditorAssetRegistry`
- `loadMapAsset(uuid)` — looks up the entry, reads the `.sasset`, reconstructs `MapAsset`

---

## Extending the System

### Adding a New Asset Type

1. Add a constant in `AssetTypeId` (e.g. `inline constexpr std::string_view MyType = "my_type";`)
2. Create a concrete `AssetBase` subclass for runtime
3. Register a factory in `RuntimeAssetRegistry`
4. (Optional) Create an `IAssetImporter` implementation
5. (Optional) Create an `IAssetCooker` implementation
6. No changes to core code required

### Adding Import Settings

1. Define a struct deriving from `AssetImportSettings`
2. Glaze auto-reflects aggregate members — no `glz::meta` needed
3. Use `SerializeImportSettings<T>()` / `ParseImportSettings<T>()` to round-trip

---

## File Map

```
src/editor/ShineAsset/
├── AssetBase.h                    Runtime asset base class
├── AssetHandle.h                  UUID-based typed reference
├── AssetTypes.h                   Well-known type ID constants
├── AssetFactory.h                 Creator function signature
├── AssetImportSettings.h          Import settings extensibility
├── AssetMetadata.h/.cpp           .sasset JSON format
├── AssetUuidHelper.h/.cpp         UUID generation/validation
├── AssetDependencyGraph.h/.cpp    Bidirectional dependency graph
├── RuntimeAssetRegistry.h/.cpp    Runtime UUID→asset map
├── EditorAssetRegistry.h/.cpp     Editor UUID→disk path map
├── EditorAssetRegistryIndex.h/.cpp Fast-startup persistent index
├── IAssetImporter.h               Import interface
├── IAssetCooker.h                 Cook interface
├── ImportPipeline.h/.cpp          Import orchestrator + auto-discovery
├── ImporterAutoRegistry.h/.cpp    Self-registration (REGISTER_IMPORTER macro)
├── CookingPipeline.h/.cpp         Cook orchestrator
├── ARCHITECTURE.md                This file
│
├── importers/                     Concrete importers (self-register, no external refs needed)
│   ├── GltfAssetImporter.h/.cpp   glTF / GLB importer
│   ├── ObjAssetImporter.h/.cpp    Wavefront OBJ importer
│   └── TextureAssetImporter.h/.cpp  PNG / JPEG texture importer
│
└── cookers/                       Concrete cookers
    └── StaticMeshCooker.h/.cpp    Model → binary mesh blob
```
