#include "ObjAssetImporter.h"
#include "ImporterAutoRegistry.h"
#include "MeshBinUtil.h"

#include <chrono>
#include <system_error>

#include <fmt/chrono.h>
#include "imgui/imgui.h"

#include "AssetTypes.h"
#include "AssetUuidHelper.h"
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
        const std::filesystem::path meshesDir = ctx.outputSAssetPath.parent_path() / "meshes";
        std::error_code ec;
        std::filesystem::create_directories(meshesDir, ec);
        if (ec)
        {
            result.errorMessage = "Failed to create meshes directory: " + meshesDir.string();
            return result;
        }

        // Build AssetMetadata
        auto& meta  = result.metadata;
        meta.formatVersion = "2.0";

        auto& asset = meta.asset;
        asset.uuid           = ctx.rootUUID;
        asset.type           = std::string(AssetTypeId::Model);
        asset.sourceFile     = ctx.sourceFile.string();
        asset.imported       = true;
        asset.lastImportTime = fmt::format("{:%FT%TZ}",
            std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
        asset.importSettings = ctx.savedImportSettings;

        // Write each mesh as a binary file and record as a sub-asset
        for (std::size_t i = 0; i < allMeshes.size(); ++i)
        {
            const auto& mesh = allMeshes[i];

            const SString meshName    = MakeMeshName(mesh.name, i);
            const SString binFilename = MakeMeshBinFilename(meshName, i);
            SString relBinPath("meshes/");
            relBinPath += binFilename;
            const auto binPath = meshesDir / binFilename.c_str();

            if (!WriteMeshBin(binPath, mesh, settings.scale, settings.flipUV))
            {
                result.errorMessage = "Failed to write mesh binary: " + binPath.string();
                return result;
            }

            SubAssetEntry sub;
            sub.uuid = GenerateV7UUIDString().to_string();
            sub.type = std::string(SubAssetTypeId::Mesh);
            sub.name = meshName.to_string();

            SString props("{\"vertexCount\":");
            props += std::to_string(mesh.vertices.size());
            props += ",\"indexCount\":";
            props += std::to_string(mesh.indices.size());
            props += ",\"materialIndex\":";
            props += std::to_string(mesh.materialIndex);
            props += ",\"binaryPath\":\"";
            props += relBinPath;
            props += "\"}";
            sub.properties = glz::raw_json{ props.to_string() };

            asset.subAssets.push_back(std::move(sub));

            if (ctx.onProgress)
            {
                const float p = 0.2f + 0.7f * (static_cast<float>(i + 1) /
                                                static_cast<float>(allMeshes.size()));
                ctx.onProgress("Writing mesh " + std::to_string(i + 1) +
                               "/" + std::to_string(allMeshes.size()), p);
            }
        }

        // Flat node tree: Root → all mesh sub-assets
        {
            AssetNode rootNode;
            rootNode.name = ctx.sourceFile.stem().string();
            for (const auto& sub : asset.subAssets)
            {
                NodeComponent comp;
                comp.type = std::string(SubAssetTypeId::Mesh);
                comp.uuid = sub.uuid;
                rootNode.components.push_back(std::move(comp));
            }
            asset.nodeTree = std::move(rootNode);
        }

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
