#include "MeshBinUtil.h"

#include <array>
#include <cstring>
#include <fstream>
#include <vector>

namespace shine::editor::asset
{
    // -------------------------------------------------------------------------
    //  File format constants
    // -------------------------------------------------------------------------
    static constexpr uint32_t kMeshBinMagic   = 0x424E4D53u; // 'SMNB'
    static constexpr uint8_t  kMeshBinVersion = 2;

    // -------------------------------------------------------------------------
    //  Software float <-> half conversion (no hardware dependency)
    // -------------------------------------------------------------------------
    static uint16_t float_to_half(float f) noexcept
    {
        uint32_t x;
        std::memcpy(&x, &f, sizeof(x));
        const uint32_t sign     = (x >> 31) & 0x1u;
        const int32_t  exp32    = static_cast<int32_t>((x >> 23) & 0xFFu) - 127;
        const uint32_t mant32   = x & 0x7FFFFFu;

        if (exp32 > 15)   return static_cast<uint16_t>((sign << 15) | 0x7C00u); // inf/overflow
        if (exp32 < -14)  return static_cast<uint16_t>(sign << 15);             // underflow → ±0
        const uint32_t exp16  = static_cast<uint32_t>(exp32 + 15);
        const uint32_t mant16 = mant32 >> 13;
        return static_cast<uint16_t>((sign << 15) | (exp16 << 10) | mant16);
    }

    static float half_to_float(uint16_t h) noexcept
    {
        const uint32_t sign  = (h >> 15) & 0x1u;
        const uint32_t exp16 = (h >> 10) & 0x1Fu;
        const uint32_t mant  = h & 0x3FFu;
        uint32_t x;
        if (exp16 == 0)
            x = (sign << 31) | (mant << 13);            // subnormal
        else if (exp16 == 31)
            x = (sign << 31) | 0x7F800000u | (mant << 13); // inf/NaN
        else
            x = (sign << 31) | ((exp16 + 112u) << 23) | (mant << 13);
        float f;
        std::memcpy(&f, &x, sizeof(f));
        return f;
    }

    // -------------------------------------------------------------------------

    SString SafeMeshFilename(STextView name)
    {
        SString s(name);
        for (char& c : s)
        {
            if (c == '/' || c == '\\' || c == ':' || c == '*' ||
                c == '?' || c == '"' || c == '<' || c == '>' ||
                c == '|' || c == ' ')
                c = '_';
        }
        return s;
    }

    SString MakeMeshName(const std::string& name, std::size_t idx)
    {
        if (!name.empty()) return SString(name);
        SString s("Mesh_");
        s += std::to_string(idx);
        return s;
    }

    SString MakeMeshBinFilename(STextView meshName, std::size_t idx)
    {
        SString result = SafeMeshFilename(meshName);
        result += "_";
        result += std::to_string(idx);
        result += ".bin";
        return result;
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

        // Header
        out.write(reinterpret_cast<const char*>(&kMeshBinMagic),   sizeof(kMeshBinMagic));
        out.write(reinterpret_cast<const char*>(&kMeshBinVersion),  sizeof(kMeshBinVersion));
        out.write(reinterpret_cast<const char*>(&vertexCount), sizeof(vertexCount));
        out.write(reinterpret_cast<const char*>(&indexCount),  sizeof(indexCount));

        // Positions — float32×3 (scale applied)
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

        // Normals — float16×3
        {
            const bool hasNormals = (mesh.normals.size() == vertexCount);
            for (uint32_t i = 0; i < vertexCount; ++i)
            {
                const auto& n = hasNormals ? mesh.normals[i] : shine::math::FVector3f(0.0f, 0.0f, 1.0f);
                const std::array<uint16_t, 3> h = {
                    float_to_half(n.X), float_to_half(n.Y), float_to_half(n.Z)
                };
                out.write(reinterpret_cast<const char*>(h.data()), h.size() * sizeof(uint16_t));
            }
        }

        // Texcoords — float16×2 (optional V-flip)
        {
            const bool hasUV = (mesh.texcoords.size() == vertexCount);
            for (uint32_t i = 0; i < vertexCount; ++i)
            {
                float u = 0.0f, v = 0.0f;
                if (hasUV) { u = mesh.texcoords[i].X; v = mesh.texcoords[i].Y; }
                if (flipUV) v = 1.0f - v;
                const std::array<uint16_t, 2> h = { float_to_half(u), float_to_half(v) };
                out.write(reinterpret_cast<const char*>(h.data()), h.size() * sizeof(uint16_t));
            }
        }

        // Indices — uint32
        out.write(reinterpret_cast<const char*>(mesh.indices.data()),
                  static_cast<std::streamsize>(indexCount * sizeof(uint32_t)));

        return out.good();
    }

    std::optional<shine::loader::MeshData> ReadMeshBin(const std::filesystem::path& path)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in) return std::nullopt;

        uint32_t magic = 0;
        uint8_t  version = 0;
        uint32_t vertexCount = 0;
        uint32_t indexCount  = 0;

        in.read(reinterpret_cast<char*>(&magic),       sizeof(magic));
        if (magic != kMeshBinMagic) return std::nullopt;
        in.read(reinterpret_cast<char*>(&version),     sizeof(version));
        if (version != kMeshBinVersion) return std::nullopt;
        in.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));
        in.read(reinterpret_cast<char*>(&indexCount),  sizeof(indexCount));
        if (!in) return std::nullopt;

        shine::loader::MeshData mesh;
        mesh.vertices.resize(vertexCount);
        mesh.normals.resize(vertexCount);
        mesh.texcoords.resize(vertexCount);
        mesh.indices.resize(indexCount);

        // Positions — float32×3
        in.read(reinterpret_cast<char*>(mesh.vertices.data()),
                static_cast<std::streamsize>(vertexCount * 3 * sizeof(float)));

        // Normals — float16×3
        for (uint32_t i = 0; i < vertexCount; ++i)
        {
            std::array<uint16_t, 3> h{};
            in.read(reinterpret_cast<char*>(h.data()), h.size() * sizeof(uint16_t));
            mesh.normals[i] = { half_to_float(h[0]), half_to_float(h[1]), half_to_float(h[2]) };
        }

        // Texcoords — float16×2
        for (uint32_t i = 0; i < vertexCount; ++i)
        {
            std::array<uint16_t, 2> h{};
            in.read(reinterpret_cast<char*>(h.data()), h.size() * sizeof(uint16_t));
            mesh.texcoords[i] = { half_to_float(h[0]), half_to_float(h[1]) };
        }

        // Indices — uint32
        in.read(reinterpret_cast<char*>(mesh.indices.data()),
                static_cast<std::streamsize>(indexCount * sizeof(uint32_t)));

        if (!in) return std::nullopt;
        return mesh;
    }

} // namespace shine::editor::asset
