#pragma once

#include "loader/core/loader.h"
#include "loader/model/model_loader.h"
#include <filesystem>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace shine::loader {

// 变量声明
struct ObjTexture {
    std::string name{};
    std::string path{};
};

struct ObjMaterial {
    std::string name;

    float Ka[3] = {0.0f, 0.0f, 0.0f}; // ambient
    float Kd[3] = {1.0f, 1.0f, 1.0f}; // diffuse
    float Ks[3] = {0.0f, 0.0f, 0.0f}; // specular
    float Ke[3] = {0.0f, 0.0f, 0.0f}; // emission
    float Kt[3] = {0.0f, 0.0f, 0.0f}; // transmittance
    float Ns    = 1.0f;               // shininess
    float Ni    = 1.0f;               // index of refraction
    float Tf[3] = {1.0f, 1.0f, 1.0f}; // transmission filter
    float d     = 1.0f;               // dissolve (alpha)
    int   illum = 1;                  // illumination model

    bool fallback = false; // true if material not from mtllib

    // texture indices into Mesh::textures (0 = none/default)
    std::size_t map_Ka   = 0;
    std::size_t map_Kd   = 0;
    std::size_t map_Ks   = 0;
    std::size_t map_Ke   = 0;
    std::size_t map_Kt   = 0;
    std::size_t map_Ns   = 0;
    std::size_t map_Ni   = 0;
    std::size_t map_d    = 0;
    std::size_t map_bump = 0;
};

// ----------------------------------------------------------------------------
// Index (face vertex)
// ----------------------------------------------------------------------------
struct ObjIndex {
    std::uint32_t p = 0; // position index (1‑based, 0 means not present)
    std::uint32_t t = 0; // texcoord index
    std::uint32_t n = 0; // normal index
};

struct ObjGroup {
    std::string name{};

    std::size_t face_count   = 0; // number of faces in this group
    std::size_t face_offset  = 0; // first face index in Mesh::face_vertices etc.
    std::size_t index_offset = 0; // first index in Mesh::indices
};

// ----------------------------------------------------------------------------
// Mesh
// ----------------------------------------------------------------------------
class ObjMesh {
public:
    // ----- vertex data ------------------------------------------------------
    std::vector<float> positions; // 3 floats per vertex
    std::vector<float> texcoords; // 2 floats per texcoord
    std::vector<float> normals;   // 3 floats per normal
    std::vector<float> colors;    // 3 floats per colour (optional)

    // ----- face data --------------------------------------------------------
    std::vector<std::uint32_t> face_vertices;  // vertices per face
    std::vector<std::uint32_t> face_materials; // material index per face
    std::vector<std::uint8_t>  face_lines;     // 1 if line, 0 if face

    // ----- index data -------------------------------------------------------
    std::vector<ObjIndex> indices; // one per face vertex

    // ----- materials --------------------------------------------------------
    std::vector<ObjMaterial> materials;

    // ----- textures ---------------------------------------------------------
    std::vector<ObjTexture> textures;

    // ----- groups and objects ------------------------------------------------
    std::vector<ObjGroup> objects;
    std::vector<ObjGroup> groups;

    // ----- convenience queries ----------------------------------------------
    [[nodiscard]] std::size_t position_count() const noexcept { return positions.size() / 3; }
    [[nodiscard]] std::size_t texcoord_count() const noexcept { return texcoords.size() / 2; }
    [[nodiscard]] std::size_t normal_count() const noexcept { return normals.size() / 3; }
    [[nodiscard]] std::size_t color_count() const noexcept { return colors.size() / 3; }
    [[nodiscard]] std::size_t face_count() const noexcept { return face_vertices.size(); }
    [[nodiscard]] std::size_t index_count() const noexcept { return indices.size(); }
    [[nodiscard]] std::size_t material_count() const noexcept { return materials.size(); }
    [[nodiscard]] std::size_t texture_count() const noexcept { return textures.size(); }
    [[nodiscard]] std::size_t object_count() const noexcept { return objects.size(); }
    [[nodiscard]] std::size_t group_count() const noexcept { return groups.size(); }

    // ----- load from file ---------------------------------------------------
    bool load(const std::filesystem::path &path);

    bool parse_mtllib(ObjMesh& mesh,
                  const std::filesystem::path& mtl_path,
                  const std::filesystem::path& base_path);

    // ----- factory ----------------------------------------------------------
    static std::unique_ptr<ObjMesh> read(const std::filesystem::path &path);

    // ----- default ctor / move only -----------------------------------------
    ObjMesh()                           = default;
    ~ObjMesh()                          = default;
    ObjMesh(const ObjMesh &)            = delete;
    ObjMesh &operator=(const ObjMesh &) = delete;
    ObjMesh(ObjMesh &&)                 = default;
    ObjMesh &operator=(ObjMesh &&)      = default;
};

class objLoader : public IModelLoader {

public:
    objLoader() {
        addSupportedExtension("obj");
    }

    virtual ~objLoader() = default;
    virtual bool loadFromMemory(const void *data, size_t size) override;
    virtual bool loadFromFile(const char *filePath) override;
    void         unload() override;

    // 实现基类方法
    virtual const char *getName() const override { return "objLoader"; }
    virtual const char *getVersion() const override { return "1.0.0"; }

    // ========================================================================
    // IModelLoader 接口实现
    // ========================================================================

    std::vector<MeshData> extractMeshData() const override;
    size_t                getMeshCount() const noexcept override;



private:
    ObjMesh Mesh{};

};

} // namespace shine::loader
