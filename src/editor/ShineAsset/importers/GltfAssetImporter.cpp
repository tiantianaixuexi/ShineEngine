#include "GltfAssetImporter.h"
#include "ImporterAutoRegistry.h"
#include "MaterialImportUtil.h"
#include "ModelImportUtil.h"

#include <fstream>
#include <system_error>

#include "imgui/imgui.h"

#include "AssetUuidHelper.h"
#include "loader/model/gltfLoader.h"

namespace shine::editor::asset
{
    // -----------------------------------------------------------------------
    //  Internal helpers
    // -----------------------------------------------------------------------
    namespace
    {
        // Convert a GltfMaterial + resolved texture UUIDs into a format-agnostic
        // MaterialImportData ready for MakeMaterialMeta().
        static MaterialImportData ToMaterialImportData(
            STextView name,
            const gltf::GltfMaterial& mat,
            const std::vector<SString>& textureUuids)
        {
            const auto resolveUuid = [&](int idx) -> SString {
                if (idx >= 0 && static_cast<size_t>(idx) < textureUuids.size())
                    return textureUuids[static_cast<size_t>(idx)];
                return {};
            };

            MaterialImportData d;
            d.name        = name;
            d.alphaMode   = STextView::from_string(mat.alphaMode);
            d.doubleSided = mat.doubleSided;

            const auto& pbr = mat.pbrMetallicRoughness;
            const auto& bcf = pbr.baseColorFactor;
            if (bcf.size() >= 4)
                d.baseColorFactor = { static_cast<float>(bcf[0]), static_cast<float>(bcf[1]),
                                      static_cast<float>(bcf[2]), static_cast<float>(bcf[3]) };
            d.metallicFactor  = static_cast<float>(pbr.metallicFactor);
            d.roughnessFactor = static_cast<float>(pbr.roughnessFactor);

            d.baseColorTextureUuid         = resolveUuid(pbr.baseColorTexture.index);
            d.metallicRoughnessTextureUuid = resolveUuid(pbr.metallicRoughnessTexture.index);
            d.normalTextureUuid            = resolveUuid(mat.normalTexture.index);
            d.emissiveTextureUuid          = resolveUuid(mat.emissiveTexture.index);
            return d;
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
        std::filesystem::path meshesDir;
        if (!CreateMeshesDir(ctx.outputSAssetPath, meshesDir, result.errorMessage))
            return result;

        const std::filesystem::path texturesDir  = assetDir / "textures";
        const std::filesystem::path materialsDir = assetDir / "materials";
        std::error_code ec;

        const SString sourceStr(ctx.sourceFile.string());

        // -----------------------------------------------------------------------
        //  Build AssetMetadata for root model
        // -----------------------------------------------------------------------
        InitModelAssetMeta(result.metadata, ctx);
        auto& asset = result.metadata.asset;

        // -----------------------------------------------------------------------
        //  Export embedded textures (images)
        // -----------------------------------------------------------------------
        const auto& gltfModel = loader.getModel();

        // textureUuids[gltfTextureIndex] = UUID of the .sasset for that texture.
        // gltf::GltfTexture.source = image index.
        std::vector<SString> textureUuids(gltfModel.textures.size());

        if (!gltfModel.images.empty())
        {
            std::filesystem::create_directories(texturesDir, ec);

            for (size_t imgIdx = 0; imgIdx < gltfModel.images.size(); ++imgIdx)
            {
                const auto& img = gltfModel.images[imgIdx];

                // Skip images with no decoded pixel data
                if (img.image.empty() || img.width == 0 || img.height == 0)
                    continue;

                const SString defaultTexName(std::string("texture_") + std::to_string(imgIdx));
                const SString stem = SafeFilenameStem(
                    img.name.empty() ? STextView(defaultTexName) : STextView(img.name),
                    defaultTexName);

                SString binFilename = stem;
                binFilename += ".bin";
                SString texSassetFilename = stem;
                texSassetFilename += ".sasset";
                const auto texBinPath    = texturesDir / binFilename.c_str();
                const auto texSassetPath = texturesDir / texSassetFilename.c_str();

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
                const SString texUuid = GenerateV7UUIDString();

                auto texMeta = MakeTextureMeta(texUuid, sourceStr, stem, binFilename,
                    static_cast<uint32_t>(img.width), static_cast<uint32_t>(img.height));

                result.sideAssets.emplace_back(texSassetPath, std::move(texMeta));
                asset.dependencies.push_back(texUuid.to_string());

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
        std::vector<SString> materialUuids(gltfModel.materials.size());

        if (!gltfModel.materials.empty())
        {
            std::filesystem::create_directories(materialsDir, ec);

            for (size_t mi = 0; mi < gltfModel.materials.size(); ++mi)
            {
                const auto& mat  = gltfModel.materials[mi];
                const SString defaultMatName(std::string("material_") + std::to_string(mi));
                const SString stem = SafeFilenameStem(
                    mat.name.empty() ? STextView(defaultMatName) : STextView::from_string(mat.name),
                    defaultMatName);

                SString matUuid = GenerateV7UUIDString();
                materialUuids[mi] = matUuid;

                SString matSassetFilename = stem;
                matSassetFilename += ".sasset";
                const auto matSassetPath = materialsDir / matSassetFilename.c_str();

                const auto matData = ToMaterialImportData(stem, mat, textureUuids);
                auto matMeta = MakeMaterialMeta(matUuid, sourceStr, matData);

                result.sideAssets.emplace_back(matSassetPath, std::move(matMeta));
                asset.dependencies.push_back(matUuid.to_string());
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

            // Resolve which material UUID this mesh uses
            const SString matUuid = (mesh.materialIndex >= 0 &&
                static_cast<size_t>(mesh.materialIndex) < materialUuids.size())
                ? materialUuids[static_cast<size_t>(mesh.materialIndex)] : SString();

            if (!WriteMeshSubAsset(asset, meshesDir, mesh, i,
                                   settings.scale, false, matUuid, result.errorMessage))
                return result;

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
            BuildFlatNodeTree(asset, "Root");

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
