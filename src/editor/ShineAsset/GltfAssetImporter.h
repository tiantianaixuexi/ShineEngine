#pragma once
// ============================================================
//  GltfAssetImporter — imports .gltf / .glb / .obj files into
//  .sasset metadata.
//
//  EDITOR-ONLY — wraps the existing gltfLoader pipeline.
// ============================================================

#include "IAssetImporter.h"

namespace shine::editor::asset
{
    class GltfAssetImporter final : public IAssetImporter
    {
    public:
        [[nodiscard]] std::string_view GetName() const noexcept override;
        [[nodiscard]] std::vector<std::string_view> SupportedExtensions() const noexcept override;
        [[nodiscard]] ImportResult Import(const AssetImportContext& ctx) override;
    };

} // namespace shine::editor::asset
