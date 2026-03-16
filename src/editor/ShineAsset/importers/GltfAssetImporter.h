#pragma once
// ============================================================
//  GltfAssetImporter — imports .gltf / .glb files into
//  .sasset metadata + per-mesh binary data files.
//
//  EDITOR-ONLY — wraps the existing gltfLoader pipeline.
//  OBJ files are handled by ObjAssetImporter.
// ============================================================

#include "IAssetImporter.h"
#include "AssetImportSettings.h"

namespace shine::editor::asset
{
    // -----------------------------------------------------------------------
    //  Type-specific import settings for glTF / glb / obj assets.
    //  Serialized as JSON via Glaze; stored in AssetRecord::importSettings.
    // -----------------------------------------------------------------------
    struct GltfImportSettings : AssetImportSettings
    {
        float scale           = 1.0f;   ///< Uniform scale applied on import.
        bool  mergeByMaterial = false;  ///< Merge meshes that share a material.
    };

    class GltfAssetImporter final : public IAssetImporter
    {
    public:
        [[nodiscard]] std::string_view GetName() const noexcept override;
        [[nodiscard]] std::vector<std::string_view> SupportedExtensions() const noexcept override;
        [[nodiscard]] ImportResult Import(const AssetImportContext& ctx) override;
        bool RenderImportSettingsUI(glz::raw_json& inOutSettings) override;
    };

} // namespace shine::editor::asset

// Explicit Glaze meta to bypass automatic aggregate reflection
// (inherited structs confuse to_tie's member-count probe).
template <>
struct glz::meta<shine::editor::asset::GltfImportSettings> {
    using T = shine::editor::asset::GltfImportSettings;
    static constexpr auto value = glz::object(
        "scale",           &T::scale,
        "mergeByMaterial", &T::mergeByMaterial
    );
};
