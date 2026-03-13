#pragma once
// ============================================================
//  AssetTypes — well-known asset type identifier constants.
//
//  Type IDs are string constants rather than a closed enum so that
//  new asset types can be introduced in any module without ever
//  modifying this file.
//
//  Convention: lower-case ASCII, underscores for multi-word types.
// ============================================================

#include <string_view>

namespace shine::editor::asset
{
    // -----------------------------------------------------------------------
    //  Well-known top-level asset type IDs.
    //  These match the "type" field in AssetRecord.
    //  External modules may define their own type ID constants; they are
    //  not required to live here.
    // -----------------------------------------------------------------------
    namespace AssetTypeId
    {
        inline constexpr std::string_view Model          = "model";
        inline constexpr std::string_view Texture        = "texture";
        inline constexpr std::string_view Material       = "material";
        inline constexpr std::string_view Audio          = "audio";
        inline constexpr std::string_view Animation      = "animation";
        inline constexpr std::string_view Script         = "script";
        inline constexpr std::string_view World          = "world";
        inline constexpr std::string_view Blueprint      = "blueprint";
        inline constexpr std::string_view RenderPipeline = "render_pipeline";
        inline constexpr std::string_view Font           = "font";
    } // namespace AssetTypeId

    // -----------------------------------------------------------------------
    //  Well-known sub-asset type IDs.
    //  These match the "type" field inside SubAssetEntry.
    // -----------------------------------------------------------------------
    namespace SubAssetTypeId
    {
        inline constexpr std::string_view Mesh      = "mesh";
        inline constexpr std::string_view Material  = "material";
        inline constexpr std::string_view Skeleton  = "skeleton";
        inline constexpr std::string_view Animation = "animation";
        inline constexpr std::string_view Texture   = "texture";
        inline constexpr std::string_view Morph     = "morph";
    } // namespace SubAssetTypeId

} // namespace shine::editor::asset
