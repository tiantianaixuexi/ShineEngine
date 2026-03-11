#pragma once

#include "EngineCore/asset/shared/AssetTypes.h"
#include "string/shine_text_view.h"

namespace shine::editor::asset
{
    class MapAssetFactory final : public shine::IAssetFactory
    {
    public:
        MapAssetFactory() = default;
        ~MapAssetFactory() override = default;

        MapAssetFactory(const MapAssetFactory&) = delete;
        MapAssetFactory& operator=(const MapAssetFactory&) = delete;
        MapAssetFactory(MapAssetFactory&&) = delete;
        MapAssetFactory& operator=(MapAssetFactory&&) = delete;

        [[nodiscard]] shine::AssetCreateResult CreateAsset(const shine::AssetCreateContext& context) override;
        [[nodiscard]] shine::EAssetKind GetSupportedKind() const noexcept override;
        [[nodiscard]] shine::STextView GetDisplayName() const noexcept override;
        [[nodiscard]] bool CanCreateNew() const noexcept override;
    };
}
