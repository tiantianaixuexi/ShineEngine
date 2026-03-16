#pragma once

#include <filesystem>
#include <string>

#include "loader/model/model_loader.h"

namespace shine::editor::asset
{
    /// Replace characters that are invalid in file names with underscores.
    std::string SafeMeshFilename(const std::string& name);

    /// Write one MeshData as a compact binary blob.
    /// Format:
    ///   [vertexCount : u32]
    ///   [indexCount  : u32]
    ///   [vertices    : float3 * vertexCount]  — scale applied
    ///   [normals     : float3 * vertexCount]  — zeros if missing
    ///   [texcoords   : float2 * vertexCount]  — zeros if missing; V optionally flipped
    ///   [indices     : u32    * indexCount ]
    bool WriteMeshBin(const std::filesystem::path& path,
                      const shine::loader::MeshData& mesh,
                      float scale,
                      bool  flipUV = false);

} // namespace shine::editor::asset
