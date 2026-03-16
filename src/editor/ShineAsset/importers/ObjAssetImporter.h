#pragma once
// ============================================================
//  ObjAssetImporter — imports .obj files into
//  .sasset metadata + per-mesh binary data files.
//
//  EDITOR-ONLY.  Uses the objLoader pipeline.
//  glTF / GLB files are handled by GltfAssetImporter.
// ============================================================

#include "IAssetImporter.h"
#include "AssetImportSettings.h"

namespace shine::editor::asset
{
    // -----------------------------------------------------------------------
    //  Type-specific import settings for OBJ assets.
    //  Serialized as JSON via Glaze; stored in AssetRecord::importSettings.
    // -----------------------------------------------------------------------
    struct ObjImportSettings : AssetImportSettings
    {
        float scale  = 1.0f;   ///< Uniform scale applied on import (unit conversion).
        bool  flipUV = false;  ///< Flip V coordinate (converts OBJ OpenGL UV to DX convention).
    };

    class ObjAssetImporter final : public IAssetImporter
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
struct glz::meta<shine::editor::asset::ObjImportSettings> {
    using T = shine::editor::asset::ObjImportSettings;
    static constexpr auto value = glz::object(
        "scale",  &T::scale,
        "flipUV", &T::flipUV
    );
};
