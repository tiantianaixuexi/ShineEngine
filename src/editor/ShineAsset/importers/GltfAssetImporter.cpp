#include "GltfAssetImporter.h"
#include "ImporterAutoRegistry.h"

#include <chrono>
#include <fstream>
#include <system_error>

#include <fmt/chrono.h>
#include "imgui/imgui.h"

#include "AssetTypes.h"
#include "AssetUuidHelper.h"
#include "loader/model/gltfLoader.h"

namespace shine::editor::asset
{
    // -----------------------------------------------------------------------
    //  Internal helpers
    // -----------------------------------------------------------------------
    namespace
    {
        /// Replace characters that are invalid in file names.
        std::string SafeFilename(const std::string& name)
        {
            std::string s = name;
            for (char& c : s)
            {
                if (c == '/' || c == '\\' || c == ':' || c == '*' ||
                    c == '?' || c == '"' || c == '<' || c == '>' ||
                    c == '|' || c == ' ')
                    c = '_';
            }
            return s;
        }

        /// Write one mesh as a compact binary blob.
        /// Format (matches StaticMeshCooker):
        ///   [vertexCount : u32]
        ///   [indexCount  : u32]
        ///   [vertices    : float3 * vertexCount]   — scale applied
        ///   [normals     : float3 * vertexCount]   — zeros if missing
        ///   [texcoords   : float2 * vertexCount]   — zeros if missing
        ///   [indices     : u32    * indexCount ]
        bool WriteMeshBin(const std::filesystem::path& path,
                          const shine::loader::MeshData& mesh,
                          float scale)
        {
            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            if (!out) return false;

            const auto vertexCount = static_cast<uint32_t>(mesh.vertices.size());
            const auto indexCount  = static_cast<uint32_t>(mesh.indices.size());

            out.write(reinterpret_cast<const char*>(&vertexCount), sizeof(vertexCount));
            out.write(reinterpret_cast<const char*>(&indexCount),  sizeof(indexCount));

            // Vertices with optional uniform scale
            if (scale != 1.0f)
            {
                for (const auto& v : mesh.vertices)
                {
                    const float sv[3] = { v.X * scale, v.Y * scale, v.Z * scale };
                    out.write(reinterpret_cast<const char*>(sv), sizeof(sv));
                }
            }
            else
            {
                out.write(reinterpret_cast<const char*>(mesh.vertices.data()),
                          static_cast<std::streamsize>(vertexCount * 3 * sizeof(float)));
            }

            // Normals
            if (mesh.normals.size() == vertexCount)
            {
                out.write(reinterpret_cast<const char*>(mesh.normals.data()),
                          static_cast<std::streamsize>(vertexCount * 3 * sizeof(float)));
            }
            else
            {
                const std::vector<float> zeros(static_cast<std::size_t>(vertexCount) * 3, 0.0f);
                out.write(reinterpret_cast<const char*>(zeros.data()),
                          static_cast<std::streamsize>(zeros.size() * sizeof(float)));
            }

            // Texcoords
            if (mesh.texcoords.size() == vertexCount)
            {
                out.write(reinterpret_cast<const char*>(mesh.texcoords.data()),
                          static_cast<std::streamsize>(vertexCount * 2 * sizeof(float)));
            }
            else
            {
                const std::vector<float> zeros(static_cast<std::size_t>(vertexCount) * 2, 0.0f);
                out.write(reinterpret_cast<const char*>(zeros.data()),
                          static_cast<std::streamsize>(zeros.size() * sizeof(float)));
            }

            // Indices
            out.write(reinterpret_cast<const char*>(mesh.indices.data()),
                      static_cast<std::streamsize>(indexCount * sizeof(uint32_t)));

            return out.good();
        }
    } // namespace

    // -----------------------------------------------------------------------
    //  IAssetImporter interface
    // -----------------------------------------------------------------------
    std::string_view GltfAssetImporter::GetName() const noexcept
    {
        return "glTF Model Importer";
    }

    std::vector<std::string_view> GltfAssetImporter::SupportedExtensions() const noexcept
    {
        return { ".gltf", ".glb" };
    }

    ImportResult GltfAssetImporter::Import(const AssetImportContext& ctx)
    {
        ImportResult result;

        // Parse import settings (fall back to defaults on first import)
        GltfImportSettings settings{};
        if (!ctx.savedImportSettings.str.empty())
        {
            if (auto parsed = ParseImportSettings<GltfImportSettings>(ctx.savedImportSettings))
                settings = *parsed;
        }

        if (ctx.onProgress)
            ctx.onProgress("Loading model file...", 0.0f);

        shine::loader::gltfLoader loader;
        if (!loader.loadFromFile(ctx.sourceFile.string().c_str()))
        {
            result.errorMessage = "Failed to load model file: " + ctx.sourceFile.string();
            return result;
        }

        if (ctx.onProgress)
            ctx.onProgress("Extracting mesh data...", 0.2f);

        auto allMeshes = loader.extractMeshData();

        // Create the meshes/ subfolder alongside the .sasset
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

        // Write each mesh as a binary file and record it as a sub-asset
        for (std::size_t i = 0; i < allMeshes.size(); ++i)
        {
            const auto& mesh = allMeshes[i];

            const std::string meshName = mesh.name.empty()
                ? "Mesh_" + std::to_string(i)
                : mesh.name;

            const std::string   binFilename = SafeFilename(meshName) + "_" + std::to_string(i) + ".bin";
            const std::string   relBinPath  = "meshes/" + binFilename;
            const auto          binPath     = meshesDir / binFilename;

            if (!WriteMeshBin(binPath, mesh, settings.scale))
            {
                result.errorMessage = "Failed to write mesh binary: " + binPath.string();
                return result;
            }

            SubAssetEntry sub;
            sub.uuid = GenerateV7UUIDString().to_string();
            sub.type = std::string(SubAssetTypeId::Mesh);
            sub.name = meshName;

            const std::string props =
                "{\"vertexCount\":"  + std::to_string(mesh.vertices.size()) +
                ",\"indexCount\":"   + std::to_string(mesh.indices.size())  +
                ",\"materialIndex\":" + std::to_string(mesh.materialIndex)  +
                ",\"binaryPath\":\""  + relBinPath + "\"}";
            sub.properties = glz::raw_json{ props };

            asset.subAssets.push_back(std::move(sub));

            if (ctx.onProgress)
            {
                const float p = 0.2f + 0.7f * (static_cast<float>(i + 1) /
                                                static_cast<float>(allMeshes.size()));
                ctx.onProgress("Writing mesh " + std::to_string(i + 1) +
                               "/" + std::to_string(allMeshes.size()), p);
            }
        }

        // Build a flat node tree (Root → all mesh sub-assets)
        if (loader.getSceneCount() > 0)
        {
            AssetNode rootNode;
            rootNode.name = "Root";
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

    bool GltfAssetImporter::RenderImportSettingsUI(glz::raw_json& inOutSettings)
    {
        GltfImportSettings settings{};
        if (!inOutSettings.str.empty())
            (void)glz::read_json(settings, inOutSettings.str);

        bool changed = false;
        changed |= ImGui::DragFloat("缩放比例", &settings.scale, 0.01f, 0.001f, 100.0f, "%.3f");
        changed |= ImGui::Checkbox("按材质合并网格", &settings.mergeByMaterial);

        if (changed)
        {
            if (auto r = SerializeImportSettings(settings); r)
                inOutSettings = std::move(*r);
        }
        return changed;
    }

    REGISTER_IMPORTER(GltfAssetImporter)

} // namespace shine::editor::asset
