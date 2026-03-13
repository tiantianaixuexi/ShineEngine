#pragma once
// ============================================================
//  IAssetCooker — interface for all concrete asset cookers.
//
//  EDITOR-ONLY — never include from runtime code.
//
//  The cook step transforms a .sasset file (editor metadata) into a
//  platform-specific runtime binary blob that the Runtime can load
//  without any editor-side code.  Each asset type (or platform) can
//  have its own cooker.
//
//  Separation from IAssetImporter is intentional:
//    Import — source file (FBX) → .sasset (human-readable metadata)
//    Cook   — .sasset          → .bin   (compact runtime binary)
//
//  This allows re-cooking without re-importing and vice-versa.
// ============================================================

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "AssetMetadata.h"

namespace shine::editor::asset
{
    // -----------------------------------------------------------------------
    //  Target platform identifier.
    //  Cookers may produce different output per platform (e.g. BC7 vs ETC2).
    // -----------------------------------------------------------------------
    enum class ECookPlatform : std::uint8_t
    {
        Windows_x64 = 0,
        Linux_x64,
        Android_ARM64,
        iOS_ARM64,
        WebAssembly,
    };

    // -----------------------------------------------------------------------
    //  Context passed to IAssetCooker::Cook().
    // -----------------------------------------------------------------------
    struct AssetCookContext
    {
        /// Parsed metadata from the .sasset file of the asset being cooked.
        AssetMetadata metadata;

        /// Target platform to produce output for.
        ECookPlatform platform = ECookPlatform::Windows_x64;

        /// Absolute root of the Content/ directory (for resolving relative paths).
        std::filesystem::path contentRoot;

        /// Output directory where the cooked binary should be written.
        std::filesystem::path outputDir;

        /// Reference to the editor registry so the cooker can look up
        /// the runtime binaries of dependency assets (e.g. cook textures
        /// before the model that references them).
        /// May be nullptr during isolated / unit-test cooks.
        class EditorAssetRegistry* editorRegistry = nullptr;

        /// Progress callback — optional.
        std::function<void(std::string_view message, float fraction)> onProgress;
    };

    // -----------------------------------------------------------------------
    //  Cook result
    // -----------------------------------------------------------------------
    struct CookResult
    {
        bool        succeeded    = false;
        std::string errorMessage;

        /// Paths of all output files written by this cook (for dependency tracking).
        std::vector<std::filesystem::path> outputFiles;
    };

    // -----------------------------------------------------------------------
    //  IAssetCooker
    // -----------------------------------------------------------------------
    class IAssetCooker
    {
    public:
        virtual ~IAssetCooker() = default;

        /// Human-readable name (e.g. "Texture Cooker (BC7)").
        [[nodiscard]] virtual std::string_view GetName() const noexcept = 0;

        /// Asset type IDs this cooker handles (matches AssetTypeId constants).
        /// e.g. { AssetTypeId::Texture }
        [[nodiscard]] virtual std::vector<std::string_view>
        SupportedTypeIds() const noexcept = 0;

        /// Return true if this cooker can handle the given platform.
        /// Default: all platforms.
        [[nodiscard]] virtual bool
        SupportsPlafform(ECookPlatform platform) const noexcept { return true; }

        /// Perform the cook.  Called on a worker thread — must be thread-safe.
        [[nodiscard]] virtual CookResult
        Cook(const AssetCookContext& ctx) = 0;
    };

} // namespace shine::editor::asset
