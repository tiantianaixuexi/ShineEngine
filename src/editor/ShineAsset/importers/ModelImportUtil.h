#pragma once
// ============================================================
//  ModelImportUtil — shared helpers for model importers
//  (OBJ, glTF, FBX, …).
//
//  EDITOR-ONLY.
// ============================================================

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "string/shine_string.h"
#include "loader/model/model_loader.h"
#include "../metadata/AssetMetadata.h"
#include "IAssetImporter.h"

namespace shine::editor::asset
{
    // -----------------------------------------------------------------------
    //  Sanitise a string for use as a filename stem.
    //  Returns `fallback` when `name` is empty.
    // -----------------------------------------------------------------------
    [[nodiscard]] SString SafeFilenameStem(STextView name, STextView fallback);

    // -----------------------------------------------------------------------
    //  Initialise the common fields of a Model-type AssetMetadata from an
    //  AssetImportContext.  Sets formatVersion, uuid, type, sourceFile,
    //  imported, lastImportTime, and importSettings.
    // -----------------------------------------------------------------------
    void InitModelAssetMeta(AssetMetadata& meta, const AssetImportContext& ctx);

    // -----------------------------------------------------------------------
    //  Create the meshes/ output directory.
    //  On success, writes the resolved path into `outMeshesDir` and returns true.
    //  On failure, fills `outError` and returns false.
    // -----------------------------------------------------------------------
    [[nodiscard]] bool CreateMeshesDir(
        const std::filesystem::path& sassetPath,
        std::filesystem::path&       outMeshesDir,
        std::string&                 outError);

    // -----------------------------------------------------------------------
    //  Write one mesh as a binary SubAssetEntry.
    //
    //  `materialUuid`  — optional material UUID to embed in properties
    //                     (empty string → omitted).
    //
    //  On success, the SubAssetEntry is appended to `asset.subAssets`.
    //  On failure, fills `outError` and returns false.
    // -----------------------------------------------------------------------
    [[nodiscard]] bool WriteMeshSubAsset(
        AssetRecord&                       asset,
        const std::filesystem::path&       meshesDir,
        const shine::loader::MeshData&     mesh,
        std::size_t                        index,
        float                              scale,
        bool                               flipUV,
        STextView                          materialUuid,
        std::string&                       outError);

    // -----------------------------------------------------------------------
    //  Build a flat node tree: a single root node whose components reference
    //  every mesh sub-asset currently stored in `asset.subAssets`.
    // -----------------------------------------------------------------------
    void BuildFlatNodeTree(AssetRecord& asset, std::string rootName);

} // namespace shine::editor::asset
