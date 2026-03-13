#include "StaticMeshCooker.h"

#include <cstring>
#include <fstream>

#include "AssetTypes.h"
#include "loader/model/gltfLoader.h"

namespace shine::editor::asset
{
    std::string_view StaticMeshCooker::GetName() const noexcept
    {
        return "Static Mesh Cooker";
    }

    std::vector<std::string_view> StaticMeshCooker::SupportedTypeIds() const noexcept
    {
        return { AssetTypeId::Model };
    }

    CookResult StaticMeshCooker::Cook(const AssetCookContext& ctx)
    {
        CookResult result;

        const auto& rec = ctx.metadata.asset;
        if (rec.sourceFile.empty())
        {
            result.errorMessage = "No source file specified in metadata";
            return result;
        }

        // Resolve source file path
        std::filesystem::path sourcePath = rec.sourceFile;
        if (sourcePath.is_relative())
            sourcePath = ctx.contentRoot / sourcePath;

        // Load the model
        shine::loader::gltfLoader loader;
        if (!loader.loadFromFile(sourcePath.string().c_str()))
        {
            result.errorMessage = "Failed to load source model: " + sourcePath.string();
            return result;
        }

        auto meshes = loader.extractMeshData();
        if (meshes.empty())
        {
            result.errorMessage = "No mesh data extracted from model";
            return result;
        }

        // Create output directory
        std::error_code ec;
        std::filesystem::create_directories(ctx.outputDir, ec);
        if (ec)
        {
            result.errorMessage = "Failed to create output directory: " + ctx.outputDir.string() + " (" + ec.message() + ")";
            return result;
        }

        // Write a compact binary mesh blob per sub-asset
        // Binary format: [vertexCount:u32][indexCount:u32][vertices:float3*N][normals:float3*N][texcoords:float2*N][indices:u32*M]
        for (std::size_t i = 0; i < meshes.size(); ++i)
        {
            const auto& mesh = meshes[i];
            std::string filename = rec.uuid + "_mesh_" + std::to_string(i) + ".bin";
            auto outputPath = ctx.outputDir / filename;

            std::ofstream out(outputPath, std::ios::binary);
            if (!out)
            {
                result.errorMessage = "Failed to create output file: " + outputPath.string();
                return result;
            }

            const uint32_t vertexCount = static_cast<uint32_t>(mesh.vertices.size());
            const uint32_t indexCount  = static_cast<uint32_t>(mesh.indices.size());

            out.write(reinterpret_cast<const char*>(&vertexCount), sizeof(vertexCount));
            out.write(reinterpret_cast<const char*>(&indexCount), sizeof(indexCount));

            // Vertices (float3 * vertexCount)
            out.write(reinterpret_cast<const char*>(mesh.vertices.data()),
                      static_cast<std::streamsize>(vertexCount * sizeof(float) * 3));

            // Normals (float3 * vertexCount)
            if (mesh.normals.size() == vertexCount)
            {
                out.write(reinterpret_cast<const char*>(mesh.normals.data()),
                          static_cast<std::streamsize>(vertexCount * sizeof(float) * 3));
            }
            else
            {
                // Write zeros for missing normals
                std::vector<float> zeros(vertexCount * 3, 0.0f);
                out.write(reinterpret_cast<const char*>(zeros.data()),
                          static_cast<std::streamsize>(zeros.size() * sizeof(float)));
            }

            // Texcoords (float2 * vertexCount)
            if (mesh.texcoords.size() == vertexCount)
            {
                out.write(reinterpret_cast<const char*>(mesh.texcoords.data()),
                          static_cast<std::streamsize>(vertexCount * sizeof(float) * 2));
            }
            else
            {
                std::vector<float> zeros(vertexCount * 2, 0.0f);
                out.write(reinterpret_cast<const char*>(zeros.data()),
                          static_cast<std::streamsize>(zeros.size() * sizeof(float)));
            }

            // Indices (u32 * indexCount)
            out.write(reinterpret_cast<const char*>(mesh.indices.data()),
                      static_cast<std::streamsize>(indexCount * sizeof(uint32_t)));

            result.outputFiles.push_back(outputPath);
        }

        if (ctx.onProgress)
            ctx.onProgress("Cook complete", 1.0f);

        result.succeeded = true;
        return result;
    }

} // namespace shine::editor::asset
