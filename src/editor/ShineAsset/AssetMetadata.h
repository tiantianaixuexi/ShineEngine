#pragma once
// ============================================================
//  AssetMetadata — editor-only metadata loaded from .sasset files.
//
//  Serialization boundary: all struct members that map to JSON use
//  std::string / std::vector / std::array / std::optional as Glaze
//  requires.  Internal engine code (runtime) must NOT include this
//  header — it is editor and cooking-pipeline only.
//
//  importSettings and SubAssetEntry::properties are stored as
//  glz::raw_json (opaque JSON bytes).  Concrete importers parse
//  them via ParseImportSettings<T> from AssetImportSettings.h.
//  Adding a new asset type never requires modifying this file.
//
//  .sasset format version: 2.0
// ============================================================

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "glaze/json.hpp"

namespace shine::editor::asset
{
    // -----------------------------------------------------------------------
    //  Transform stored in the node tree (translation / rotation / scale)
    //  rotation is a quaternion stored as [x, y, z, w].
    // -----------------------------------------------------------------------
    struct AssetTransform
    {
        std::array<float, 3> translation = {0.f, 0.f, 0.f};
        std::array<float, 4> rotation    = {0.f, 0.f, 0.f, 1.f};
        std::array<float, 3> scale       = {1.f, 1.f, 1.f};
    };

    // -----------------------------------------------------------------------
    //  A typed component reference inside a scene-graph node.
    //  "type" matches SubAssetTypeId constants; "uuid" points to a
    //  SubAssetEntry or an external asset.
    // -----------------------------------------------------------------------
    struct NodeComponent
    {
        std::string type;
        std::string uuid;
    };

    // -----------------------------------------------------------------------
    //  One node in the hierarchical scene graph embedded in the asset.
    //  Recursive via std::vector<AssetNode>.
    // -----------------------------------------------------------------------
    struct AssetNode
    {
        std::string                name;
        AssetTransform             transform;
        std::vector<NodeComponent> components;
        std::vector<AssetNode>     children;
    };

    // -----------------------------------------------------------------------
    //  A sub-asset (mesh, material, skeleton, animation clip, …).
    //
    //  `properties` is a raw JSON blob so the metadata base layer has
    //  zero coupling to concrete asset types.  Concrete importers parse
    //  it by calling glz::read_json<MyProperties>(entry.properties.str).
    // -----------------------------------------------------------------------
    struct SubAssetEntry
    {
        std::string  uuid;
        std::string  type;         // matches SubAssetTypeId constants
        std::string  name;
        glz::raw_json properties;  // opaque; parsed by concrete importer
    };

    // -----------------------------------------------------------------------
    //  Freeform user metadata attached to an asset.
    //  `extra` stores any additional key-value pairs not covered by `tags`.
    // -----------------------------------------------------------------------
    struct AssetUserData
    {
        std::vector<std::string> tags;
        glz::raw_json            extra;  // e.g. {"author":"ArtistName"}
    };

    // -----------------------------------------------------------------------
    //  Core asset record — the "asset" JSON object in a .sasset file.
    //
    //  importSettings: raw JSON blob, type-specific.  Callers retrieve
    //  typed settings by calling:
    //      ParseImportSettings<MySettings>(record.importSettings)
    // -----------------------------------------------------------------------
    struct AssetRecord
    {
        std::string  uuid;
        std::string  type;           // matches AssetTypeId constants
        std::string  sourceFile;
        bool         imported       = false;
        std::string  lastImportTime;

        // Opaque; parsed by the concrete importer via AssetImportSettings.h
        glz::raw_json              importSettings;

        std::vector<SubAssetEntry> subAssets;
        std::optional<AssetNode>   nodeTree;
        std::vector<std::string>   dependencies;
        AssetUserData              userData;
    };

    // -----------------------------------------------------------------------
    //  Top-level .sasset document
    // -----------------------------------------------------------------------
    struct AssetMetadata
    {
        std::string formatVersion = "2.0";
        AssetRecord asset;
    };

    // -----------------------------------------------------------------------
    //  Read / write helpers
    // -----------------------------------------------------------------------

    /// Parse a .sasset JSON string (or string_view) into AssetMetadata.
    /// Unknown keys are silently ignored for forward compatibility.
    [[nodiscard]] glz::expected<AssetMetadata, glz::error_ctx>
    ReadAssetMetadata(std::string_view json);

    /// Read a .sasset file from disk.
    [[nodiscard]] glz::expected<AssetMetadata, glz::error_ctx>
    ReadAssetMetadataFile(std::string_view filePath);

    /// Serialize AssetMetadata to a prettified JSON string.
    [[nodiscard]] glz::expected<std::string, glz::error_ctx>
    WriteAssetMetadata(const AssetMetadata& meta);

    /// Write AssetMetadata to a .sasset file.
    [[nodiscard]] glz::expected<std::string, glz::error_ctx>
    WriteAssetMetadataFile(const AssetMetadata& meta, std::string_view filePath);

} // namespace shine::editor::asset
