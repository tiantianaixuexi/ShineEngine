#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "loader/model/model_loader.h"

namespace shine::editor::asset
{
    /// Replace characters that are invalid in file names with underscores.
    std::string SafeMeshFilename(const std::string& name);

    /// Write one MeshData as a compact binary blob.
    /// Format (version 2):
    ///   [magic   : u32 = 0x424E4D53 'SMNB']
    ///   [version : u8  = 2              ]
    ///   [vertexCount : u32              ]
    ///   [indexCount  : u32              ]
    ///   [vertices    : float32×3 * vertexCount]  — scale applied
    ///   [normals     : float16×3 * vertexCount]  — zeros if missing
    ///   [texcoords   : float16×2 * vertexCount]  — zeros if missing; V optionally flipped
    ///   [indices     : u32       * indexCount ]
    bool WriteMeshBin(const std::filesystem::path& path,
                      const shine::loader::MeshData& mesh,
                      float scale,
                      bool  flipUV = false);

    /// Read a .bin written by WriteMeshBin back into a MeshData.
    /// Returns nullopt on any read or format error.
    std::optional<shine::loader::MeshData> ReadMeshBin(const std::filesystem::path& path);

} // namespace shine::editor::asset
