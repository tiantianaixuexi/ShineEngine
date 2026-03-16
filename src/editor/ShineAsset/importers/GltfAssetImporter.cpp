#include "GltfAssetImporter.h"
#include "ImporterAutoRegistry.h"
#include "MeshBinUtil.h"

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
        // Sanitise a string for use as a filename stem.
        static std::string SafeStem(const std::string& name, const std::string& fallback)
        {
            if (name.empty()) return fallback;
            std::string s = name;
            for (char& c : s)
            {
                if (c == '/' || c == '\\' || c == ':' || c == '*' ||
                    c == '?' || c == '"'  || c == '<' || c == '>'  ||
                    c == '|' || c == ' ')
                    c = '_';
            }
            return s;
        }

        // Write raw RGBA pixels to a simple 4-channel .bin (matches TextureAssetImporter format).
        static bool WriteTextureBin(const std::filesystem::path& path,
                                    uint32_t width, uint32_t height,
                                    const std::vector<unsigned char>& rgba)
        {
            std::filesystem::create_directories(path.parent_path());
            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            if (!out) return false;
            constexpr uint32_t channels = 4u;
            out.write(reinterpret_cast<const char*>(&width),    sizeof(width));
            out.write(reinterpret_cast<const char*>(&height),   sizeof(height));
            out.write(reinterpret_cast<const char*>(&channels), sizeof(channels));
            out.write(reinterpret_cast<const char*>(rgba.data()),
                      static_cast<std::streamsize>(rgba.size()));
            return out.good();
        }

        // Build a texture AssetMetadata (same structure as TextureAssetImporter).
        static AssetMetadata MakeTextureMeta(
            const std::string& uuid,
            const std::string& sourceFile,
            const std::string& name,
            const std::string& binFilename,
            uint32_t width, uint32_t height)
        {
            AssetMetadata meta;
            meta.formatVersion   = "2.0";
            meta.asset.uuid      = uuid;
            meta.asset.type      = std::string(AssetTypeId::Texture);
            meta.asset.sourceFile = sourceFile;
            meta.asset.imported  = true;
            meta.asset.lastImportTime = fmt::format("{:%FT%TZ}",
                std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));

            SubAssetEntry sub;
            sub.uuid = GenerateV7UUIDString().to_string();
            sub.type = std::string(SubAssetTypeId::Texture);
            sub.name = name;
            const std::string props =
                "{\"width\":"     + std::to_string(width)  +
                ",\"height\":"    + std::to_string(height) +
                ",\"channels\":4"
                ",\"generateMipmaps\":true"
                ",\"sRGB\":true"
                ",\"binaryPath\":\"" + binFilename + "\"}";
            sub.properties = glz::raw_json{ props };
            meta.asset.subAssets.push_back(std::move(sub));
            return meta;
        }

        // Build a material AssetMetadata with PBR properties and texture UUID refs.
        static AssetMetadata MakeMaterialMeta(
            const std::string& uuid,
            const std::string& sourceFile,
            const std::string& name,
            const gltf::GltfMaterial& mat,
            const std::vector<std::string>& textureUuids)   // indexed by gltf texture index
        {
            const auto texUuid = [&](int idx) -> std::string {
                if (idx >= 0 && static_cast<size_t>(idx) < textureUuids.size())
                    return textureUuids[static_cast<size_t>(idx)];
                return "";
            };

            AssetMetadata meta;
            meta.formatVersion   = "2.0";
            meta.asset.uuid      = uuid;
            meta.asset.type      = std::string(AssetTypeId::Material);
            meta.asset.sourceFile = sourceFile;
            meta.asset.imported  = true;
            meta.asset.lastImportTime = fmt::format("{:%FT%TZ}",
                std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));

            const auto& pbr = mat.pbrMetallicRoughness;
            const auto& bcf = pbr.baseColorFactor;

            // Build properties JSON
            const std::string baseColorUuid = texUuid(pbr.baseColorTexture.index);
            const std::string mrUuid        = texUuid(pbr.metallicRoughnessTexture.index);
            const std::string normalUuid    = texUuid(mat.normalTexture.index);
            const std::string emissiveUuid  = texUuid(mat.emissiveTexture.index);

            std::string props = "{";
            props += "\"name\":\"" + name + "\"";
            props += ",\"alphaMode\":\"" + mat.alphaMode + "\"";
            props += std::string(",\"doubleSided\":") + (mat.doubleSided ? "true" : "false");
            if (bcf.size() >= 4)
            {
                props += ",\"baseColorFactor\":["
                    + std::to_string(bcf[0]) + "," + std::to_string(bcf[1]) + ","
                    + std::to_string(bcf[2]) + "," + std::to_string(bcf[3]) + "]";
            }
            props += ",\"metallicFactor\":"  + std::to_string(pbr.metallicFactor);
            props += ",\"roughnessFactor\":" + std::to_string(pbr.roughnessFactor);
            if (!baseColorUuid.empty()) props += ",\"baseColorTextureUuid\":\"" + baseColorUuid + "\"";
            if (!mrUuid.empty())        props += ",\"metallicRoughnessTextureUuid\":\"" + mrUuid + "\"";
            if (!normalUuid.empty())    props += ",\"normalTextureUuid\":\"" + normalUuid + "\"";
            if (!emissiveUuid.empty())  props += ",\"emissiveTextureUuid\":\"" + emissiveUuid + "\"";
            props += "}";

            SubAssetEntry sub;
            sub.uuid = GenerateV7UUIDString().to_string();
            sub.type = std::string(SubAssetTypeId::Material);
            sub.name = name;
            sub.properties = glz::raw_json{ props };
            meta.asset.subAssets.push_back(std::move(sub));
            return meta;
        }
    } // anonymous namespace

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

        if (allMeshes.empty())
        {
            result.errorMessage = "No renderable meshes found in: " + ctx.sourceFile.filename().string()
                + " (file may use unsupported primitive modes or contain no geometry)";
            return result;
        }

        const std::filesystem::path assetDir    = ctx.outputSAssetPath.parent_path();
        const std::filesystem::path meshesDir   = assetDir / "meshes";
        const std::filesystem::path texturesDir = assetDir / "textures";
        const std::filesystem::path materialsDir = assetDir / "materials";

        std::error_code ec;
        std::filesystem::create_directories(meshesDir, ec);
        if (ec)
        {
            result.errorMessage = "Failed to create meshes directory: " + meshesDir.string();
            return result;
        }

        const std::string sourceStr = ctx.sourceFile.string();
        const std::string nowStr    = fmt::format("{:%FT%TZ}",
            std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));

        // -----------------------------------------------------------------------
        //  Build AssetMetadata for root model
        // -----------------------------------------------------------------------
        auto& meta  = result.metadata;
        meta.formatVersion = "2.0";

        auto& asset = meta.asset;
        asset.uuid           = ctx.rootUUID;
        asset.type           = std::string(AssetTypeId::Model);
        asset.sourceFile     = sourceStr;
        asset.imported       = true;
        asset.lastImportTime = nowStr;
        asset.importSettings = ctx.savedImportSettings;

        // -----------------------------------------------------------------------
        //  Export embedded textures (images)
        // -----------------------------------------------------------------------
        const auto& gltfModel = loader.getModel();

        // textureUuids[gltfTextureIndex] = UUID of the .sasset for that texture.
        // gltf::GltfTexture.source = image index.
        std::vector<std::string> textureUuids(gltfModel.textures.size());

        if (!gltfModel.images.empty())
        {
            std::filesystem::create_directories(texturesDir, ec);

            for (size_t imgIdx = 0; imgIdx < gltfModel.images.size(); ++imgIdx)
            {
                const auto& img = gltfModel.images[imgIdx];

                // Skip images with no decoded pixel data
                if (img.image.empty() || img.width == 0 || img.height == 0)
                    continue;

                const std::string stem = SafeStem(img.name.empty()
                    ? "texture_" + std::to_string(imgIdx)
                    : img.name.to_string(), "texture_" + std::to_string(imgIdx));

                const std::string binFilename  = stem + ".bin";
                const auto        texBinPath   = texturesDir / binFilename;
                const auto        texSassetPath = texturesDir / (stem + ".sasset");

                // Convert to RGBA (component may be 3 or 4)
                std::vector<unsigned char> rgba;
                const size_t numPixels = static_cast<size_t>(img.width) * img.height;
                rgba.resize(numPixels * 4);
                for (size_t px = 0; px < numPixels; ++px)
                {
                    size_t srcBase = px * static_cast<size_t>(img.component);
                    rgba[px * 4 + 0] = img.component > 0 ? img.image[srcBase + 0] : 0;
                    rgba[px * 4 + 1] = img.component > 1 ? img.image[srcBase + 1] : 0;
                    rgba[px * 4 + 2] = img.component > 2 ? img.image[srcBase + 2] : 0;
                    rgba[px * 4 + 3] = img.component > 3 ? img.image[srcBase + 3] : 255;
                }

                WriteTextureBin(texBinPath,
                    static_cast<uint32_t>(img.width), static_cast<uint32_t>(img.height), rgba);

                // Map each gltf texture that references this image to the same UUID
                const std::string texUuid = GenerateV7UUIDString().to_string();

                auto texMeta = MakeTextureMeta(texUuid, sourceStr, stem, binFilename,
                    static_cast<uint32_t>(img.width), static_cast<uint32_t>(img.height));

                result.sideAssets.emplace_back(texSassetPath, std::move(texMeta));
                asset.dependencies.push_back(texUuid);

                // Fill textureUuids for all gltf textures referencing this image
                for (size_t ti = 0; ti < gltfModel.textures.size(); ++ti)
                {
                    if (gltfModel.textures[ti].source == static_cast<int>(imgIdx))
                        textureUuids[ti] = texUuid;
                }
            }
        }

        if (ctx.onProgress)
            ctx.onProgress("Exporting materials...", 0.4f);

        // -----------------------------------------------------------------------
        //  Export materials
        // -----------------------------------------------------------------------
        // materialUuids[gltfMaterialIndex] = UUID
        std::vector<std::string> materialUuids(gltfModel.materials.size());

        if (!gltfModel.materials.empty())
        {
            std::filesystem::create_directories(materialsDir, ec);

            for (size_t mi = 0; mi < gltfModel.materials.size(); ++mi)
            {
                const auto& mat  = gltfModel.materials[mi];
                const std::string stem = SafeStem(mat.name.empty()
                    ? "material_" + std::to_string(mi)
                    : mat.name, "material_" + std::to_string(mi));

                const std::string matUuid = GenerateV7UUIDString().to_string();
                materialUuids[mi] = matUuid;

                const auto matSassetPath = materialsDir / (stem + ".sasset");

                auto matMeta = MakeMaterialMeta(matUuid, sourceStr, stem, mat, textureUuids);
                matMeta.asset.lastImportTime = nowStr;

                result.sideAssets.emplace_back(matSassetPath, std::move(matMeta));
                asset.dependencies.push_back(matUuid);
            }
        }

        if (ctx.onProgress)
            ctx.onProgress("Writing mesh data...", 0.5f);

        // -----------------------------------------------------------------------
        //  Write mesh binary files and record sub-assets
        // -----------------------------------------------------------------------
        for (std::size_t i = 0; i < allMeshes.size(); ++i)
        {
            const auto& mesh = allMeshes[i];

            const std::string meshName = mesh.name.empty()
                ? "Mesh_" + std::to_string(i)
                : mesh.name;

            const std::string   binFilename = SafeMeshFilename(meshName) + "_" + std::to_string(i) + ".bin";
            const std::string   relBinPath  = "meshes/" + binFilename;
            const auto          binPath     = meshesDir / binFilename;

            if (!WriteMeshBin(binPath, mesh, settings.scale))
            {
                result.errorMessage = "Failed to write mesh binary: " + binPath.string();
                return result;
            }

            // Resolve which material UUID this mesh uses
            const std::string matUuid = (mesh.materialIndex >= 0 &&
                static_cast<size_t>(mesh.materialIndex) < materialUuids.size())
                ? materialUuids[static_cast<size_t>(mesh.materialIndex)] : "";

            SubAssetEntry sub;
            sub.uuid = GenerateV7UUIDString().to_string();
            sub.type = std::string(SubAssetTypeId::Mesh);
            sub.name = meshName;

            std::string props =
                "{\"vertexCount\":"  + std::to_string(mesh.vertices.size()) +
                ",\"indexCount\":"   + std::to_string(mesh.indices.size())  +
                ",\"materialIndex\":" + std::to_string(mesh.materialIndex)  +
                ",\"binaryPath\":\""  + relBinPath + "\"" +
                ",\"materialUuids\":[" + (matUuid.empty() ? "" : "\"" + matUuid + "\"") + "]}";
            sub.properties = glz::raw_json{ props };

            asset.subAssets.push_back(std::move(sub));

            if (ctx.onProgress)
            {
                const float p = 0.5f + 0.45f * (static_cast<float>(i + 1) /
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
