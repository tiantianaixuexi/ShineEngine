#pragma once
// ============================================================
//  StaticMeshCooker — cooks model assets into compact binary blobs.
//
//  EDITOR-ONLY.
// ============================================================

#include "IAssetCooker.h"

namespace shine::editor::asset
{
    class StaticMeshCooker final : public IAssetCooker
    {
    public:
        [[nodiscard]] std::string_view GetName() const noexcept override;
        [[nodiscard]] std::vector<std::string_view> SupportedTypeIds() const noexcept override;
        [[nodiscard]] CookResult Cook(const AssetCookContext& ctx) override;
    };

} // namespace shine::editor::asset
