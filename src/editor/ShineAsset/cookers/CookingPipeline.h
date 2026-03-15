#pragma once
// ============================================================
//  CookingPipeline — drives the asset cooking process.
//
//  EDITOR-ONLY.
//
//  Iterates all entries in EditorAssetRegistry, finds the
//  appropriate IAssetCooker for each type, and produces
//  platform-specific runtime binaries.
// ============================================================

#include <memory>
#include <unordered_map>
#include <vector>

#include "IAssetCooker.h"
#include "string/shine_string.h"

namespace shine::editor::asset
{
    class EditorAssetRegistry;

    class CookingPipeline
    {
    public:
        CookingPipeline() = default;

        /// Register a cooker for the type IDs it supports.
        void RegisterCooker(std::shared_ptr<IAssetCooker> cooker);

        /// Cook all assets in the registry for the given platform.
        /// Returns the number of assets successfully cooked.
        std::size_t CookAll(
            EditorAssetRegistry& registry,
            ECookPlatform platform,
            const std::filesystem::path& contentRoot,
            const std::filesystem::path& outputDir);

        /// Cook a single asset by UUID.
        [[nodiscard]] CookResult CookSingle(
            EditorAssetRegistry& registry,
            STextView uuid,
            ECookPlatform platform,
            const std::filesystem::path& contentRoot,
            const std::filesystem::path& outputDir);

    private:
        /// type ID → cooker
        std::unordered_map<SString, std::shared_ptr<IAssetCooker>> m_cookers;
    };

} // namespace shine::editor::asset
