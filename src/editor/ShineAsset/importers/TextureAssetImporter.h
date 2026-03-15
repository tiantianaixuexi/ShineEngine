#pragma once
// ============================================================
//  TextureAssetImporter — imports .png / .jpg / .jpeg files into
//  .sasset metadata + RGBA pixel binary data file.
//
//  EDITOR-ONLY.  Uses the built-in PNG and JPEG decoders.
// ============================================================

#include "IAssetImporter.h"
#include "AssetImportSettings.h"

namespace shine::editor::asset
{
    // -----------------------------------------------------------------------
    //  Type-specific import settings for texture assets.
    //  Serialized as JSON via Glaze; stored in AssetRecord::importSettings.
    // -----------------------------------------------------------------------
    struct TextureImportSettings : AssetImportSettings
    {
        bool generateMipmaps = true;  ///< Hint for the cooker / runtime to generate mipmaps.
        bool sRGB            = true;  ///< Interpret pixels as sRGB (linear for normal/data maps).
    };

    class TextureAssetImporter final : public IAssetImporter
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
struct glz::meta<shine::editor::asset::TextureImportSettings> {
    using T = shine::editor::asset::TextureImportSettings;
    static constexpr auto value = glz::object(
        "generateMipmaps", &T::generateMipmaps,
        "sRGB",            &T::sRGB
    );
};
