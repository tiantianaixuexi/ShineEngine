#pragma once
// ============================================================
//  MaterialImportUtil — format-agnostic material / texture
//  asset-building helpers shared by all mesh importers
//  (GLTF, GLB, OBJ, FBX, …).
//
//  EDITOR-ONLY.
// ============================================================

#include <array>
#include <filesystem>
#include <vector>

#include "string/shine_string.h"
#include "../metadata/AssetMetadata.h"

namespace shine::editor::asset
{
    // -----------------------------------------------------------------------
    //  Texture reference within a material (just the UUID of the .sasset).
    //  Empty string = no texture bound.
    // -----------------------------------------------------------------------
    using TextureUuidList = std::vector<SString>;

    // -----------------------------------------------------------------------
    //  Format-agnostic PBR material description.
    //  Populate from GltfMaterial / FbxMaterial / … before calling
    //  MakeMaterialMeta().
    // -----------------------------------------------------------------------
    struct MaterialImportData
    {
        SString name;
        SString alphaMode         = "OPAQUE";
        bool    doubleSided       = false;

        std::array<float, 4> baseColorFactor    = {1.f, 1.f, 1.f, 1.f};
        float                metallicFactor      = 1.f;
        float                roughnessFactor     = 1.f;

        // UUIDs referencing the .sasset of the bound textures (empty = unbound).
        SString baseColorTextureUuid;
        SString metallicRoughnessTextureUuid;
        SString normalTextureUuid;
        SString emissiveTextureUuid;
    };

    // -----------------------------------------------------------------------
    //  Write raw RGBA pixel data as a 4-channel .bin file.
    //  Format:
    //    [width    : u32]
    //    [height   : u32]
    //    [channels : u32]  — always 4
    //    [pixels   : u8 * width * height * 4]
    //  Returns false on any I/O error.
    // -----------------------------------------------------------------------
    [[nodiscard]] bool WriteTextureBin(
        const std::filesystem::path&        path,
        uint32_t                            width,
        uint32_t                            height,
        const std::vector<unsigned char>&   rgba);

    // -----------------------------------------------------------------------
    //  Build a texture AssetMetadata (.sasset) for a single RGBA image.
    //  `binFilename`  — relative name of the .bin pixel file (e.g. "albedo.bin").
    //  `uuid`         — pre-generated UUID for the root AssetRecord.
    //  `sourceFile`   — absolute path of the original source asset.
    // -----------------------------------------------------------------------
    [[nodiscard]] AssetMetadata MakeTextureMeta(
        STextView uuid,
        STextView sourceFile,
        STextView name,
        STextView binFilename,
        uint32_t  width,
        uint32_t  height,
        bool      generateMipmaps = true,
        bool      sRGB            = true);

    // -----------------------------------------------------------------------
    //  Build a material AssetMetadata (.sasset) from a MaterialImportData.
    //  `uuid`         — pre-generated UUID for the root AssetRecord.
    //  `sourceFile`   — absolute path of the original source asset.
    // -----------------------------------------------------------------------
    [[nodiscard]] AssetMetadata MakeMaterialMeta(
        STextView                 uuid,
        STextView                 sourceFile,
        const MaterialImportData& data);

} // namespace shine::editor::asset
