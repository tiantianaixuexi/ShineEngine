#include "GltfAssetImporter.h"

#include <chrono>
#include <fmt/chrono.h>

#include "AssetTypes.h"
#include "AssetUuidHelper.h"
#include "loader/model/gltfLoader.h"

namespace shine::editor::asset
{
    std::string_view GltfAssetImporter::GetName() const noexcept
    {
        return "glTF Model Importer";
    }

    std::vector<std::string_view> GltfAssetImporter::SupportedExtensions() const noexcept
    {
        return { ".gltf", ".glb", ".obj" };
    }

    ImportResult GltfAssetImporter::Import(const AssetImportContext& ctx)
    {
        ImportResult result;

        if (ctx.onProgress)
            ctx.onProgress("Loading model file...", 0.0f);

        // Load the model using the existing gltfLoader
        shine::loader::gltfLoader loader;
        if (!loader.loadFromFile(ctx.sourceFile.string().c_str()))
        {
            result.succeeded = false;
            result.errorMessage = "Failed to load model file: " + ctx.sourceFile.string();
            return result;
        }

        if (ctx.onProgress)
            ctx.onProgress("Extracting mesh data...", 0.3f);

        // Build AssetMetadata
        auto& meta = result.metadata;
        meta.formatVersion = "2.0";

        auto& asset = meta.asset;
        asset.uuid       = ctx.rootUUID;
        asset.type       = std::string(AssetTypeId::Model);
        asset.sourceFile  = ctx.sourceFile.string();
        asset.imported   = true;
        asset.lastImportTime = fmt::format("{:%FT%TZ}",
            std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));

        // Preserve import settings if provided
        asset.importSettings = ctx.savedImportSettings;

        // Extract mesh data as sub-assets
        const std::size_t meshCount = loader.getMeshCount();
        auto allMeshes = loader.extractMeshData();

        for (std::size_t i = 0; i < allMeshes.size(); ++i)
        {
            SubAssetEntry sub;
            sub.uuid = GenerateV7UUIDString().to_string();
            sub.type = std::string(SubAssetTypeId::Mesh);
            sub.name = allMeshes[i].name.empty()
                ? "Mesh_" + std::to_string(i)
                : allMeshes[i].name;

            // Store basic mesh properties as raw JSON
            std::string props = "{\"vertexCount\":" + std::to_string(allMeshes[i].vertices.size())
                + ",\"indexCount\":" + std::to_string(allMeshes[i].indices.size())
                + ",\"materialIndex\":" + std::to_string(allMeshes[i].materialIndex) + "}";
            sub.properties = glz::raw_json{ std::move(props) };

            asset.subAssets.push_back(std::move(sub));

            if (ctx.onProgress)
            {
                float progress = 0.3f + 0.6f * (static_cast<float>(i + 1) / static_cast<float>(allMeshes.size()));
                ctx.onProgress("Processing mesh " + std::to_string(i + 1) + "/" + std::to_string(allMeshes.size()), progress);
            }
        }

        // Build node tree from scene graph
        const int defaultScene = loader.getDefaultSceneIndex();
        if (loader.getSceneCount() > 0)
        {
            const int sceneIdx = (defaultScene >= 0) ? defaultScene : 0;
            auto rootNodes = loader.getSceneRootNodes(sceneIdx);

            AssetNode rootNode;
            rootNode.name = "Root";
            for (std::size_t i = 0; i < rootNodes.size() && i < asset.subAssets.size(); ++i)
            {
                NodeComponent comp;
                comp.type = std::string(SubAssetTypeId::Mesh);
                comp.uuid = asset.subAssets[i].uuid;
                rootNode.components.push_back(std::move(comp));
            }
            asset.nodeTree = std::move(rootNode);
        }

        if (ctx.onProgress)
            ctx.onProgress("Import complete", 1.0f);

        result.succeeded = true;
        return result;
    }

} // namespace shine::editor::asset
