#include "ModelImportUtil.h"
#include "MeshBinUtil.h"

#include <chrono>
#include <system_error>

#include <fmt/chrono.h>

#include "AssetTypes.h"
#include "AssetUuidHelper.h"

namespace shine::editor::asset
{
    SString SafeFilenameStem(STextView name, STextView fallback)
    {
        if (name.empty()) return SString(fallback);
        return SafeMeshFilename(name);
    }

    void InitModelAssetMeta(AssetMetadata& meta, const AssetImportContext& ctx)
    {
        meta.formatVersion = "2.0";

        auto& asset = meta.asset;
        asset.uuid           = ctx.rootUUID;
        asset.type           = std::string(AssetTypeId::Model);
        asset.sourceFile     = ctx.sourceFile.string();
        asset.imported       = true;
        asset.lastImportTime = fmt::format("{:%FT%TZ}",
            std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
        asset.importSettings = ctx.savedImportSettings;
    }

    bool CreateMeshesDir(
        const std::filesystem::path& sassetPath,
        std::filesystem::path&       outMeshesDir,
        std::string&                 outError)
    {
        outMeshesDir = sassetPath.parent_path() / "meshes";
        std::error_code ec;
        std::filesystem::create_directories(outMeshesDir, ec);
        if (ec)
        {
            outError = "Failed to create meshes directory: " + outMeshesDir.string();
            return false;
        }
        return true;
    }

    bool WriteMeshSubAsset(
        AssetRecord&                       asset,
        const std::filesystem::path&       meshesDir,
        const shine::loader::MeshData&     mesh,
        std::size_t                        index,
        float                              scale,
        bool                               flipUV,
        STextView                          materialUuid,
        std::string&                       outError)
    {
        const SString meshName    = MakeMeshName(mesh.name, index);
        const SString binFilename = MakeMeshBinFilename(meshName, index);
        SString relBinPath("meshes/");
        relBinPath += binFilename;
        const auto binPath = meshesDir / binFilename.c_str();

        if (!WriteMeshBin(binPath, mesh, scale, flipUV))
        {
            outError = "Failed to write mesh binary: " + binPath.string();
            return false;
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
        if (!materialUuid.empty())
        {
            props += "\",\"materialUuids\":[\"";
            props += materialUuid;
            props += "\"]}";
        }
        else
        {
            props += "\"}";
        }

        sub.properties = glz::raw_json{ props.to_string() };
        asset.subAssets.push_back(std::move(sub));
        return true;
    }

    void BuildFlatNodeTree(AssetRecord& asset, std::string rootName)
    {
        AssetNode rootNode;
        rootNode.name = std::move(rootName);
        for (const auto& sub : asset.subAssets)
        {
            NodeComponent comp;
            comp.type = std::string(SubAssetTypeId::Mesh);
            comp.uuid = sub.uuid;
            rootNode.components.push_back(std::move(comp));
        }
        asset.nodeTree = std::move(rootNode);
    }

} // namespace shine::editor::asset
