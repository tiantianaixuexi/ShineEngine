#include "MeshBinUtil.h"

#include <array>
#include <fstream>
#include <vector>

namespace shine::editor::asset
{
    std::string SafeMeshFilename(const std::string& name)
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

    bool WriteMeshBin(const std::filesystem::path& path,
                      const shine::loader::MeshData& mesh,
                      float scale,
                      bool  flipUV)
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
                const std::array<float, 3> sv = { v.X * scale, v.Y * scale, v.Z * scale };
                out.write(reinterpret_cast<const char*>(sv.data()), sv.size() * sizeof(float));
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

        // Texcoords — optional V-flip for DX convention
        if (mesh.texcoords.size() == vertexCount)
        {
            if (flipUV)
            {
                for (const auto& uv : mesh.texcoords)
                {
                    const std::array<float, 2> flipped = { uv.X, 1.0f - uv.Y };
                    out.write(reinterpret_cast<const char*>(flipped.data()), flipped.size() * sizeof(float));
                }
            }
            else
            {
                out.write(reinterpret_cast<const char*>(mesh.texcoords.data()),
                          static_cast<std::streamsize>(vertexCount * 2 * sizeof(float)));
            }
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

} // namespace shine::editor::asset
