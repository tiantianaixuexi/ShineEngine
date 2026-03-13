#pragma once
// ============================================================
//  EditorAssetRegistryIndex — fast-startup persistent index.
//
//  EDITOR-ONLY — never include from runtime code.
//
//  Persists the EditorAssetRegistry to a single JSON index file
//  (e.g. Content/.assetindex) so startup is fast: instead of
//  re-scanning and re-parsing every .sasset file, the editor loads
//  this compact index and only re-scans for files that have changed
//  since the last write (detected via last-modified timestamp).
//
//  Index format:
//  {
//    "indexVersion": 1,
//    "writtenAt": "2026-03-13T12:00:00Z",
//    "entries": [
//      {
//        "uuid": "...",
//        "diskPath": "Content/Models/Character.sasset",
//        "type": "model",
//        "dependencies": ["uuid1", "uuid2"],
//        "lastModified": 1741870800   // Unix timestamp (seconds)
//      },
//      ...
//    ]
//  }
// ============================================================

#include <filesystem>

#include "EditorAssetRegistry.h"

namespace shine::editor::asset
{
    // -----------------------------------------------------------------------
    //  Index entry — a lightweight snapshot of each registry entry.
    //  Full AssetRecord is NOT stored here (only what's needed for fast
    //  startup validation and dependency graph reconstruction).
    // -----------------------------------------------------------------------
    struct RegistryIndexEntry
    {
        std::string              uuid;
        std::string              diskPath;
        std::string              type;
        std::vector<std::string> dependencies;
        std::int64_t             lastModified = 0;  // Unix seconds
    };

    struct RegistryIndexDocument
    {
        int                            indexVersion = 1;
        std::string                    writtenAt;
        std::vector<RegistryIndexEntry> entries;
    };

    // -----------------------------------------------------------------------
    //  Save / Load
    // -----------------------------------------------------------------------

    /// Serialize the registry to a compact JSON index file.
    /// `indexPath` — typically Content/.assetindex
    /// Returns false on I/O or serialization error.
    [[nodiscard]] bool SaveRegistryIndex(
        const EditorAssetRegistry&   registry,
        const std::filesystem::path& indexPath);

    /// Load a registry index and populate `registry`.
    /// For each entry whose .sasset file has changed since `lastModified`,
    /// emits the path via `outStaleFiles` — the caller should re-import those.
    /// Returns false on I/O or parse error.
    [[nodiscard]] bool LoadRegistryIndex(
        EditorAssetRegistry&              registry,
        const std::filesystem::path&      indexPath,
        std::vector<std::filesystem::path>& outStaleFiles);

} // namespace shine::editor::asset
