#include "ObjAssetImporter.h"
#include "ImporterAutoRegistry.h"
#include "ModelImportUtil.h"

#include "imgui/imgui.h"

#include "loader/model/objLoader.h"

namespace shine::editor::asset
{
    // -----------------------------------------------------------------------
    //  IAssetImporter interface
    // -----------------------------------------------------------------------
    std::string_view ObjAssetImporter::GetName() const noexcept
    {
        return "OBJ Model Importer";
    }

    std::vector<std::string_view> ObjAssetImporter::SupportedExtensions() const noexcept
    {
        return { ".obj" };
    }

    ImportResult ObjAssetImporter::Import(const AssetImportContext& ctx)
    {
        ImportResult result;

        // Parse import settings (fall back to defaults on first import)
        ObjImportSettings settings{};
        if (!ctx.savedImportSettings.str.empty())
        {
            if (auto parsed = ParseImportSettings<ObjImportSettings>(ctx.savedImportSettings))
                settings = *parsed;
        }

        if (ctx.onProgress)
            ctx.onProgress("Loading OBJ file...", 0.0f);

        shine::loader::objLoader loader;
        if (!loader.loadFromFile(ctx.sourceFile.string().c_str()))
        {
            result.errorMessage = "Failed to load OBJ file: " + ctx.sourceFile.string();
            return result;
        }

        if (ctx.onProgress)
            ctx.onProgress("Extracting mesh data...", 0.2f);

        auto allMeshes = loader.extractMeshData();
        if (allMeshes.empty())
        {
            result.errorMessage = "No mesh data found in OBJ file: " + ctx.sourceFile.string();
            return result;
        }

        // Create meshes/ subfolder alongside the .sasset
        std::filesystem::path meshesDir;
        if (!CreateMeshesDir(ctx.outputSAssetPath, meshesDir, result.errorMessage))
            return result;

        // Build AssetMetadata
        InitModelAssetMeta(result.metadata, ctx);
        auto& asset = result.metadata.asset;

        // Write each mesh as a binary file and record as a sub-asset
        for (std::size_t i = 0; i < allMeshes.size(); ++i)
        {
            if (!WriteMeshSubAsset(asset, meshesDir, allMeshes[i], i,
                                   settings.scale, settings.flipUV, {}, result.errorMessage))
                return result;

            if (ctx.onProgress)
            {
                const float p = 0.2f + 0.7f * (static_cast<float>(i + 1) /
                                                static_cast<float>(allMeshes.size()));
                ctx.onProgress("Writing mesh " + std::to_string(i + 1) +
                               "/" + std::to_string(allMeshes.size()), p);
            }
        }

        // Flat node tree: Root → all mesh sub-assets
        BuildFlatNodeTree(asset, ctx.sourceFile.stem().string());

        if (ctx.onProgress)
            ctx.onProgress("Import complete", 1.0f);

        result.succeeded = true;
        return result;
    }

    bool ObjAssetImporter::RenderImportSettingsUI(glz::raw_json& inOutSettings)
    {
        ObjImportSettings settings{};
        if (!inOutSettings.str.empty())
            (void)glz::read_json(settings, inOutSettings.str);

        bool changed = false;
        changed |= ImGui::DragFloat("缩放比例", &settings.scale, 0.01f, 0.001f, 100.0f, "%.3f");
        changed |= ImGui::Checkbox("翻转 V 坐标 (DX 惯例)", &settings.flipUV);

        if (changed)
        {
            if (auto r = SerializeImportSettings(settings); r)
                inOutSettings = std::move(*r);
        }
        return changed;
    }

    REGISTER_IMPORTER(ObjAssetImporter)

} // namespace shine::editor::asset
