#include "ImportPipeline.h"

#include <filesystem>

#include "AssetMetadata.h"
#include "AssetUuidHelper.h"
#include "EditorAssetRegistry.h"
#include "ImporterAutoRegistry.h"

namespace shine::editor::asset
{
    bool ImportPipeline::Init(shine::EngineContext& /*ctx*/)
    {
        // Pull every importer that registered itself via REGISTER_IMPORTER().
        // This runs after all static initialisers have completed, so the
        // registry is fully populated at this point.
        for (auto& imp : ImporterAutoRegistry::Instance().CreateAll())
            m_importers.push_back(std::move(imp));
        return true;
    }

    void ImportPipeline::RegisterImporter(std::shared_ptr<IAssetImporter> importer)
    {
        m_importers.push_back(std::move(importer));
    }

    IAssetImporter* ImportPipeline::FindImporter(const std::filesystem::path& sourceFile) const noexcept
    {
        for (const auto& imp : m_importers)
        {
            if (imp->CanImport(sourceFile))
                return imp.get();
        }
        return nullptr;
    }

    ImportResult ImportPipeline::ExecuteImport(
        IAssetImporter&               importer,
        const std::filesystem::path&  sourceFile,
        const std::filesystem::path&  destDir,
        const std::filesystem::path&  contentRoot,
        glz::raw_json                 savedSettings,
        EditorAssetRegistry*          registry)
    {
        // Each imported asset lives in its own named subfolder to keep binary
        // data and metadata organised: destDir/<stem>/<stem>.sasset
        const std::string             stem        = sourceFile.stem().string();
        const std::filesystem::path   assetDir    = destDir / stem;
        const std::filesystem::path   outputSAsset = assetDir / (stem + ".sasset");

        AssetImportContext ctx;
        ctx.sourceFile          = sourceFile;
        ctx.contentRoot         = contentRoot;
        ctx.outputSAssetPath    = outputSAsset;
        ctx.rootUUID            = GenerateV7UUIDString().to_string();
        ctx.savedImportSettings = std::move(savedSettings);

        ImportResult result = importer.Import(ctx);
        if (!result.succeeded)
            return result;

        std::error_code ec;
        std::filesystem::create_directories(outputSAsset.parent_path(), ec);

        auto writeResult = WriteAssetMetadataFile(result.metadata, outputSAsset.string());
        if (!writeResult)
        {
            result.succeeded    = false;
            result.errorMessage = "Failed to write .sasset: " + outputSAsset.string();
            return result;
        }

        if (registry)
            registry->Register(outputSAsset, result.metadata.asset);

        return result;
    }

} // namespace shine::editor::asset
