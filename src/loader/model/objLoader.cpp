#include "objLoader.h"
#include <unordered_set>

#include <algorithm>
#include <charconv>
#include <functional>
#include <string>
#include <vector>
#include <fstream>


#include "fast_float/fast_float.h"

#include "fmt/format.h"

#include "math/rotator.h"
#include "math/vector.ixx"
#include "math/vector2.h"


#include "util/file_util.ixx"
#include "util/string_util.ixx"
#include "util/timer/function_timer.h"


namespace shine::loader {
using namespace shine::math;

inline const char* skip_whitespace(const char* ptr) noexcept
{
    while (*ptr == ' ' || *ptr == '\t' || *ptr == '\r')
        ++ptr;
    return ptr;
}

// Skip until end of line (including the newline)
inline const char* skip_line(const char* ptr) noexcept
{
    while (*ptr && *ptr != '\n')
        ++ptr;
    if (*ptr == '\n')
        ++ptr;
    return ptr;
}


static
int is_newline(char c)
{
    return (c == '\n');
}

inline bool parse_int(const char*& ptr, int& out) noexcept
{
    auto [end, ec] = std::from_chars(ptr, ptr + strlen(ptr), out);
    if (ec != std::errc())
        return false;
    ptr = end;
    return true;
}

inline std::size_t find_or_add_texture(std::vector<ObjTexture>& textures,
                                        const std::string& name,
                                        const std::filesystem::path& base_path)
{
    // textures[0] is reserved (empty)
    for (std::size_t i = 1; i < textures.size(); ++i)
        if (textures[i].name == name)
            return i;

    // not found – create new
    ObjTexture tex;
    tex.name = name;
    tex.path = (base_path / name).lexically_normal().string();
    textures.push_back(std::move(tex));
    return textures.size() - 1;
}

inline bool parse_float(const char*& ptr, float& out) noexcept
{
    
    auto [end, ec] = fast_float::from_chars(ptr, ptr + strlen(ptr), out);
    if (ec != std::errc())
        return false;
    ptr = end;
    return true;
}


// Helper to find a material by name (linear search)
inline std::size_t find_material(const std::vector<ObjMaterial>& materials, std::string_view name) noexcept
{
    for (std::size_t i = 0; i < materials.size(); ++i)
        if (materials[i].name == name)
            return i;
    return static_cast<std::size_t>(-1);
}

std::unique_ptr<ObjMesh> ObjMesh::read(const std::filesystem::path &path) {
    util::FunctionTimer __timer(fmt::format("parse obj file : {}",path.string()));

    auto mesh = std::make_unique<ObjMesh>();
    if (mesh->load(path))
        return mesh;
    return nullptr;
}

bool ObjMesh::load(const std::filesystem::path &path) {

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return false;


    // Base directory for resolving relative paths
    std::filesystem::path base_dir = path.parent_path();

    // Initialise with dummy elements at index 0 (so that indices 1..N are valid)
    positions = {0.0f, 0.0f, 0.0f};
    texcoords = {0.0f, 0.0f};
    normals   = {0.0f, 0.0f, 1.0f};
    textures.emplace_back();   // dummy texture

    // Current parsing state
    ObjGroup current_object;
    ObjGroup current_group;
    std::size_t current_material = 0;   // material index (0 = default)

    // Helper lambdas to flush object/group when they have faces
    auto flush_object = [&]()
    {
        if (current_object.face_count > 0)
            objects.push_back(std::move(current_object));
        else
            current_object.name.clear();

        current_object = ObjGroup{};
        current_object.face_offset  = face_vertices.size();
        current_object.index_offset = indices.size();
    };

    auto flush_group = [&]()
    {
        if (current_group.face_count > 0)
            groups.push_back(std::move(current_group));
        else
            current_group.name.clear();

        current_group = ObjGroup{};
        current_group.face_offset  = face_vertices.size();
        current_group.index_offset = indices.size();
    };


    std::string line;
    while (std::getline(file, line))
    {
        const char* ptr = line.c_str();
        ptr = skip_whitespace(ptr);
        if (*ptr == '#' || *ptr == '\0')
            continue;

        // --------------------------------------------------------------------
        // Vertex data
        // --------------------------------------------------------------------
        if (ptr[0] == 'v' && isspace(ptr[1]))
        {
            ptr += 1;
            float x, y, z;
            if (!parse_float(ptr, x)) continue;
            if (!parse_float(ptr, y)) continue;
            if (!parse_float(ptr, z)) continue;
            positions.push_back(x);
            positions.push_back(y);
            positions.push_back(z);

            // optional colours (r,g,b) – if present, fill colors array to match position count
            ptr = skip_whitespace(ptr);
            if (*ptr && !is_newline(*ptr))
            {
                // Ensure colors array has same number of elements as positions
                while (colors.size() < positions.size() - 3)
                    colors.push_back(1.0f);   // pad with white

                float r, g, b;
                if (parse_float(ptr, r) && parse_float(ptr, g) && parse_float(ptr, b))
                {
                    colors.push_back(r);
                    colors.push_back(g);
                    colors.push_back(b);
                }
            }
        }
        else if (ptr[0] == 'v' && ptr[1] == 't' && isspace(ptr[2]))
        {
            ptr += 2;
            float u, v;
            if (!parse_float(ptr, u)) continue;
            if (!parse_float(ptr, v)) continue;
            texcoords.push_back(u);
            texcoords.push_back(v);
        }
        else if (ptr[0] == 'v' && ptr[1] == 'n' && isspace(ptr[2]))
        {
            ptr += 2;
            float x, y, z;
            if (!parse_float(ptr, x)) continue;
            if (!parse_float(ptr, y)) continue;
            if (!parse_float(ptr, z)) continue;
            normals.push_back(x);
            normals.push_back(y);
            normals.push_back(z);
        }
        // --------------------------------------------------------------------
        // Face / line
        // --------------------------------------------------------------------
        else if (ptr[0] == 'f' && isspace(ptr[1]))
        {
            ptr += 1;
            std::size_t first_index = indices.size();
            std::uint32_t vcount = 0;

            while (true)
            {
                ptr = skip_whitespace(ptr);
                if (*ptr == '\n' || *ptr == '\0')
                    break;

                int p = 0, t = 0, n = 0;
                if (!parse_int(ptr, p))
                    break;   // invalid index, skip this face

                if (*ptr == '/')
                {
                    ++ptr;
                    if (*ptr != '/')
                        parse_int(ptr, t);
                    if (*ptr == '/')
                    {
                        ++ptr;
                        parse_int(ptr, n);
                    }
                }

                // Convert to absolute indices (1‑based, negative means relative to end)
                std::uint32_t ip, it, in;
                if (p > 0)
                    ip = static_cast<std::uint32_t>(p);
                else if (p < 0)
                    ip = static_cast<std::uint32_t>(position_count() + p + 1);  // p is negative
                else
                    continue;   // zero index invalid

                if (t > 0)
                    it = static_cast<std::uint32_t>(t);
                else if (t < 0)
                    it = static_cast<std::uint32_t>(texcoord_count() + t + 1);
                else
                    it = 0;

                if (n > 0)
                    in = static_cast<std::uint32_t>(n);
                else if (n < 0)
                    in = static_cast<std::uint32_t>(normal_count() + n + 1);
                else
                    in = 0;

                indices.push_back({ip, it, in});
                ++vcount;
            }

            if (vcount >= 3)   // at least a triangle
            {
                face_vertices.push_back(vcount);
                face_materials.push_back(static_cast<std::uint32_t>(current_material));
                face_lines.push_back(0);   // not a line

                current_group.face_count++;
                current_object.face_count++;
            }
            else
            {
                // discard indices we just added
                indices.resize(first_index);
            }
        }
        else if (ptr[0] == 'l' && isspace(ptr[1]))
        {
            ptr += 1;
            std::size_t first_index = indices.size();
            std::uint32_t vcount = 0;

            while (true)
            {
                ptr = skip_whitespace(ptr);
                if (*ptr == '\n' || *ptr == '\0')
                    break;

                int p = 0, t = 0, n = 0;
                if (!parse_int(ptr, p))
                    break;

                if (*ptr == '/')
                {
                    ++ptr;
                    if (*ptr != '/')
                        parse_int(ptr, t);
                    if (*ptr == '/')
                    {
                        ++ptr;
                        parse_int(ptr, n);
                    }
                }

                std::uint32_t ip, it, in;
                if (p > 0)
                    ip = static_cast<std::uint32_t>(p);
                else if (p < 0)
                    ip = static_cast<std::uint32_t>(position_count() + p + 1);
                else
                    continue;

                if (t > 0)
                    it = static_cast<std::uint32_t>(t);
                else if (t < 0)
                    it = static_cast<std::uint32_t>(texcoord_count() + t + 1);
                else
                    it = 0;

                if (n > 0)
                    in = static_cast<std::uint32_t>(n);
                else if (n < 0)
                    in = static_cast<std::uint32_t>(normal_count() + n + 1);
                else
                    in = 0;

                indices.push_back({ip, it, in});
                ++vcount;
            }

            if (vcount >= 2)   // at least a line
            {
                face_vertices.push_back(vcount);
                face_materials.push_back(static_cast<std::uint32_t>(current_material));
                face_lines.push_back(1);   // line

                current_group.face_count++;
                current_object.face_count++;
            }
            else
            {
                indices.resize(first_index);
            }
        }
        // --------------------------------------------------------------------
        // Group / Object
        // --------------------------------------------------------------------
        else if (ptr[0] == 'o' && isspace(ptr[1]))
        {
            ptr += 1;
            ptr = skip_whitespace(ptr);
            const char* start = ptr;
            while (*ptr && !isspace(static_cast<unsigned char>(*ptr)) && *ptr != '\r')
                ++ptr;
            flush_object();
            current_object.name.assign(start, ptr - start);
        }
        else if (ptr[0] == 'g' && isspace(ptr[1]))
        {
            ptr += 1;
            ptr = skip_whitespace(ptr);
            const char* start = ptr;
            while (*ptr && !isspace(static_cast<unsigned char>(*ptr)) && *ptr != '\r')
                ++ptr;
            flush_group();
            current_group.name.assign(start, ptr - start);
        }
        // --------------------------------------------------------------------
        // Material library
        // --------------------------------------------------------------------
        else if (std::strncmp(ptr, "mtllib", 6) == 0 && isspace(ptr[6]))
        {
            ptr += 6;
            ptr = skip_whitespace(ptr);
            const char* start = ptr;
            while (*ptr && !isspace(static_cast<unsigned char>(*ptr)) && *ptr != '\r')
                ++ptr;
            std::string lib_name(start, ptr - start);
            std::filesystem::path mtl_path = base_dir / lib_name;
            parse_mtllib(*this, mtl_path, base_dir);
        }
        // --------------------------------------------------------------------
        // Material assignment
        // --------------------------------------------------------------------
        else if (std::strncmp(ptr, "usemtl", 6) == 0 && isspace(ptr[6]))
        {
            ptr += 6;
            ptr = skip_whitespace(ptr);
            const char* start = ptr;
            while (*ptr && !isspace(static_cast<unsigned char>(*ptr)) && *ptr != '\r')
                ++ptr;
            std::string mtl_name(start, ptr - start);

            auto idx = find_material(materials, mtl_name);
            if (idx == static_cast<std::size_t>(-1))
            {
                // material not found – create fallback
                ObjMaterial fallback;
                fallback.name = mtl_name;
                fallback.fallback = true;
                materials.push_back(std::move(fallback));
                idx = materials.size() - 1;
            }
            current_material = idx;
        }
    }

    // Flush final object/group
    flush_object();
    flush_group();

    // Ensure colors array has same length as positions (pad with 1.0f)
    if (!colors.empty())
    {
        while (colors.size() < positions.size())
            colors.push_back(1.0f);
    }

    return true;
}

bool ObjMesh::parse_mtllib(ObjMesh &mesh, const std::filesystem::path &mtl_path, const std::filesystem::path &base_path) {
    std::ifstream file(mtl_path, std::ios::binary);
    if (!file.is_open())
        return false;

    std::string line;
    ObjMaterial current;   // starts as default
    bool have_material = false;

    while (std::getline(file, line))
    {
        const char* ptr = line.c_str();
        ptr = skip_whitespace(ptr);
        if (*ptr == '#' || *ptr == '\0')
            continue;

        // newmtl
        if (std::strncmp(ptr, "newmtl", 6) == 0 && isspace(ptr[6]))
        {
            // push previous material
            if (have_material)
            {
                mesh.materials.push_back(std::move(current));
                current = ObjMaterial{};
            }

            ptr += 6;
            ptr = skip_whitespace(ptr);
            const char* name_start = ptr;
            while (*ptr && !isspace(static_cast<unsigned char>(*ptr)) && *ptr != '\r')
                ++ptr;
            current.name.assign(name_start, ptr - name_start);
            have_material = true;
            continue;
        }

        if (!have_material)
            continue;   // ignore commands before first newmtl

        // Ka, Kd, Ks, Ke, Kt
        if (ptr[0] == 'K' && ptr[1] == 'a' && isspace(ptr[2]))
        {
            ptr += 2;
            parse_float(ptr, current.Ka[0]);
            parse_float(ptr, current.Ka[1]);
            parse_float(ptr, current.Ka[2]);
        }
        else if (ptr[0] == 'K' && ptr[1] == 'd' && isspace(ptr[2]))
        {
            ptr += 2;
            parse_float(ptr, current.Kd[0]);
            parse_float(ptr, current.Kd[1]);
            parse_float(ptr, current.Kd[2]);
        }
        else if (ptr[0] == 'K' && ptr[1] == 's' && isspace(ptr[2]))
        {
            ptr += 2;
            parse_float(ptr, current.Ks[0]);
            parse_float(ptr, current.Ks[1]);
            parse_float(ptr, current.Ks[2]);
        }
        else if (ptr[0] == 'K' && ptr[1] == 'e' && isspace(ptr[2]))
        {
            ptr += 2;
            parse_float(ptr, current.Ke[0]);
            parse_float(ptr, current.Ke[1]);
            parse_float(ptr, current.Ke[2]);
        }
        else if (ptr[0] == 'K' && ptr[1] == 't' && isspace(ptr[2]))
        {
            ptr += 2;
            parse_float(ptr, current.Kt[0]);
            parse_float(ptr, current.Kt[1]);
            parse_float(ptr, current.Kt[2]);
        }
        // Ns, Ni
        else if (ptr[0] == 'N' && ptr[1] == 's' && isspace(ptr[2]))
        {
            ptr += 2;
            parse_float(ptr, current.Ns);
        }
        else if (ptr[0] == 'N' && ptr[1] == 'i' && isspace(ptr[2]))
        {
            ptr += 2;
            parse_float(ptr, current.Ni);
        }
        // Tr (transparency) – optional, we prefer d
        else if (ptr[0] == 'T' && ptr[1] == 'r' && isspace(ptr[2]))
        {
            float tr;
            ptr += 2;
            if (parse_float(ptr, tr))
            {
                // Only set d if not already set (d overrides Tr)
                if (current.d == 1.0f)  // still default?
                    current.d = 1.0f - tr;
            }
        }
        // Tf
        else if (ptr[0] == 'T' && ptr[1] == 'f' && isspace(ptr[2]))
        {
            ptr += 2;
            parse_float(ptr, current.Tf[0]);
            parse_float(ptr, current.Tf[1]);
            parse_float(ptr, current.Tf[2]);
        }
        // d (dissolve)
        else if (ptr[0] == 'd' && isspace(ptr[1]))
        {
            ptr += 1;
            parse_float(ptr, current.d);
        }
        // illum
        else if (std::strncmp(ptr, "illum", 5) == 0 && isspace(ptr[5]))
        {
            ptr += 5;
            int i;
            if (parse_int(ptr, i))
                current.illum = i;
        }
        // map_Ka, map_Kd, ...
        else if (std::strncmp(ptr, "map_Ka", 6) == 0 && isspace(ptr[6]))
        {
            ptr += 6;
            ptr = skip_whitespace(ptr);
            const char* start = ptr;
            while (*ptr && !isspace(static_cast<unsigned char>(*ptr)) && *ptr != '\r')
                ++ptr;
            std::string tex_name(start, ptr - start);
            current.map_Ka = find_or_add_texture(mesh.textures, tex_name, base_path);
        }
        else if (std::strncmp(ptr, "map_Kd", 6) == 0 && isspace(ptr[6]))
        {
            ptr += 6;
            ptr = skip_whitespace(ptr);
            const char* start = ptr;
            while (*ptr && !isspace(static_cast<unsigned char>(*ptr)) && *ptr != '\r')
                ++ptr;
            std::string tex_name(start, ptr - start);
            current.map_Kd = find_or_add_texture(mesh.textures, tex_name, base_path);
        }
        else if (std::strncmp(ptr, "map_Ks", 6) == 0 && isspace(ptr[6]))
        {
            ptr += 6;
            ptr = skip_whitespace(ptr);
            const char* start = ptr;
            while (*ptr && !isspace(static_cast<unsigned char>(*ptr)) && *ptr != '\r')
                ++ptr;
            std::string tex_name(start, ptr - start);
            current.map_Ks = find_or_add_texture(mesh.textures, tex_name, base_path);
        }
        else if (std::strncmp(ptr, "map_Ke", 6) == 0 && isspace(ptr[6]))
        {
            ptr += 6;
            ptr = skip_whitespace(ptr);
            const char* start = ptr;
            while (*ptr && !isspace(static_cast<unsigned char>(*ptr)) && *ptr != '\r')
                ++ptr;
            std::string tex_name(start, ptr - start);
            current.map_Ke = find_or_add_texture(mesh.textures, tex_name, base_path);
        }
        else if (std::strncmp(ptr, "map_Kt", 6) == 0 && isspace(ptr[6]))
        {
            ptr += 6;
            ptr = skip_whitespace(ptr);
            const char* start = ptr;
            while (*ptr && !isspace(static_cast<unsigned char>(*ptr)) && *ptr != '\r')
                ++ptr;
            std::string tex_name(start, ptr - start);
            current.map_Kt = find_or_add_texture(mesh.textures, tex_name, base_path);
        }
        else if (std::strncmp(ptr, "map_Ns", 6) == 0 && isspace(ptr[6]))
        {
            ptr += 6;
            ptr = skip_whitespace(ptr);
            const char* start = ptr;
            while (*ptr && !isspace(static_cast<unsigned char>(*ptr)) && *ptr != '\r')
                ++ptr;
            std::string tex_name(start, ptr - start);
            current.map_Ns = find_or_add_texture(mesh.textures, tex_name, base_path);
        }
        else if (std::strncmp(ptr, "map_Ni", 6) == 0 && isspace(ptr[6]))
        {
            ptr += 6;
            ptr = skip_whitespace(ptr);
            const char* start = ptr;
            while (*ptr && !isspace(static_cast<unsigned char>(*ptr)) && *ptr != '\r')
                ++ptr;
            std::string tex_name(start, ptr - start);
            current.map_Ni = find_or_add_texture(mesh.textures, tex_name, base_path);
        }
        else if (std::strncmp(ptr, "map_d", 5) == 0 && isspace(ptr[5]))
        {
            ptr += 5;
            ptr = skip_whitespace(ptr);
            const char* start = ptr;
            while (*ptr && !isspace(static_cast<unsigned char>(*ptr)) && *ptr != '\r')
                ++ptr;
            std::string tex_name(start, ptr - start);
            current.map_d = find_or_add_texture(mesh.textures, tex_name, base_path);
        }
        else if (std::strncmp(ptr, "map_bump", 8) == 0 && isspace(ptr[8]))
        {
            ptr += 8;
            ptr = skip_whitespace(ptr);
            const char* start = ptr;
            while (*ptr && !isspace(static_cast<unsigned char>(*ptr)) && *ptr != '\r')
                ++ptr;
            std::string tex_name(start, ptr - start);
            current.map_bump = find_or_add_texture(mesh.textures, tex_name, base_path);
        }
        else if (std::strncmp(ptr, "bump", 4) == 0 && isspace(ptr[4]))   // alternative
        {
            ptr += 4;
            ptr = skip_whitespace(ptr);
            const char* start = ptr;
            while (*ptr && !isspace(static_cast<unsigned char>(*ptr)) && *ptr != '\r')
                ++ptr;
            std::string tex_name(start, ptr - start);
            current.map_bump = find_or_add_texture(mesh.textures, tex_name, base_path);
        }
    }

    if (have_material)
        mesh.materials.push_back(std::move(current));

    return true;
}

bool objLoader::loadFromMemory(const void *data, size_t size) {
    return false;
}

bool objLoader::loadFromFile(const char *filePath) {
    return false;
}

void objLoader::unload() {
}

std::vector<MeshData> objLoader::extractMeshData() const {
    return std::vector<MeshData>();
}

size_t objLoader::getMeshCount() const noexcept {
    return size_t();
}

} // namespace shine::loader
