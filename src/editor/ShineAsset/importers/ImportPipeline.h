#pragma once
// ============================================================
//  ImportPipeline — registry of IAssetImporter instances.
//
//  EDITOR-ONLY.
//
//  Resolves the correct importer for a given source file by extension,
//  and drives the full import sequence:
//      Import() → WriteAssetMetadataFile() → EditorAssetRegistry::Register()
//
//  Adding a new asset type requires only:
//    1. Implement a new IAssetImporter subclass.
//    2. Call RegisterImporter() at startup (e.g. in EditorCompositionRoot).
//  The browser and pipeline need NO changes.
// ============================================================

#include <filesystem>
#include <memory>
#include <vector>

#include "IAssetImporter.h"
#include "ImporterAutoRegistry.h"
#include "EngineCore/subsystem.h"

namespace shine::editor::asset
{
    class EditorAssetRegistry;

    class ImportPipeline : public shine::Subsystem
    {
    public:
        ImportPipeline() = default;

        /// Subsystem hook: automatically loads every importer that used
        /// REGISTER_IMPORTER(), then appends any manually registered ones.
        bool Init(shine::EngineContext& ctx) override;

        /// Register a concrete importer manually (e.g. from plugin code).
        /// Safe to call before or after Init().
        void RegisterImporter(std::shared_ptr<IAssetImporter> importer);

        /// Return the first registered importer whose CanImport() returns true
        /// for `sourceFile`, or nullptr if none is found.
        [[nodiscard]] IAssetImporter*
        FindImporter(const std::filesystem::path& sourceFile) const noexcept;

        /// Full import sequence:
        ///   1. Build AssetImportContext from arguments.
        ///   2. Call importer.Import(ctx).
        ///   3. Write the resulting .sasset to destDir/<stem>.sasset.
        ///   4. Register the new entry in `registry` (if non-null).
        /// Returns the ImportResult (.succeeded / .errorMessage).
        [[nodiscard]] ImportResult ExecuteImport(
            IAssetImporter&               importer,
            const std::filesystem::path&  sourceFile,
            const std::filesystem::path&  destDir,
            const std::filesystem::path&  contentRoot,
            glz::raw_json                 savedSettings,
            EditorAssetRegistry*          registry);

    private:
        std::vector<std::shared_ptr<IAssetImporter>> m_importers;
    };

} // namespace shine::editor::asset
