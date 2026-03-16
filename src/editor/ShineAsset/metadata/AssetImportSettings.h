#pragma once
// ============================================================
//  AssetImportSettings — extensibility point for asset pipeline.
//
//  To add import settings for a new asset type:
//    1. Define an aggregate struct that publicly derives from
//       AssetImportSettings anywhere in your module:
//
//         struct MyTypeImportSettings : shine::editor::asset::AssetImportSettings
//         {
//             float myParam = 1.0f;
//             bool  myFlag  = true;
//             // Glaze auto-reflects members — no glz::meta needed.
//         };
//
//    2. Use SerializeImportSettings<MyTypeImportSettings>(settings)
//       to write into AssetRecord::importSettings before saving.
//
//    3. Use ParseImportSettings<MyTypeImportSettings>(record.importSettings)
//       inside your importer to get a typed struct back.
//
//  This file has ZERO knowledge of any concrete asset type.
//  Adding a new type NEVER requires editing this file.
// ============================================================

#include <concepts>
#include <string>

#include "glaze/json.hpp"

namespace shine::editor::asset
{
    // -----------------------------------------------------------------------
    //  Marker base — derive from this to declare a valid import settings type.
    // -----------------------------------------------------------------------
    struct AssetImportSettings {};

    // Concept: valid import settings must publicly derive from AssetImportSettings.
    template<typename T>
    concept ImportSettingsConcept = std::derived_from<T, AssetImportSettings>;

    // -----------------------------------------------------------------------
    //  Round-trip helpers: typed struct  ↔  raw glz::raw_json
    //
    //  glz::raw_json stores the JSON bytes verbatim; Glaze writes/reads
    //  the field inline without re-wrapping.  This is the correct type
    //  for opaque JSON blobs embedded inside a larger serialized struct.
    // -----------------------------------------------------------------------

    /// Serialize typed import settings into a raw JSON blob for storage in
    /// AssetRecord::importSettings.
    template<ImportSettingsConcept T>
    [[nodiscard]] glz::expected<glz::raw_json, glz::error_ctx>
    SerializeImportSettings(const T& settings)
    {
        auto result = glz::write_json(settings);
        if (!result)
            return glz::unexpected(result.error());
        return glz::raw_json{ std::move(result.value()) };
    }

    /// Deserialize typed import settings from the raw JSON blob stored in
    /// AssetRecord::importSettings.  Call this inside your concrete importer.
    template<ImportSettingsConcept T>
    [[nodiscard]] glz::expected<T, glz::error_ctx>
    ParseImportSettings(const glz::raw_json& raw)
    {
        T out{};
        if (auto ec = glz::read_json(out, raw.str); ec)
            return glz::unexpected(ec);
        return out;
    }

} // namespace shine::editor::asset
