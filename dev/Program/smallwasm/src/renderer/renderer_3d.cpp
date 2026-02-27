#include "renderer_3d.h"
#include "../util/wasm_compat.h"
#include "../util/sort_utils.h"

namespace shine {
namespace renderer {

static Renderer3D s_renderer3d;

Renderer3D& Renderer3D::instance() {
    return s_renderer3d;
}

void Renderer3D::reset() {
    m_depth.clear();
    m_opaque.clear();
    m_transparent.clear();
    m_emissive.clear();
}

void Renderer3D::begin_frame() {
    reset();
}

int Renderer3D::create_mesh(int vao, int vertexCount, int primitive) {
    Mesh m;
    m.vao = vao;
    m.vertexCount = vertexCount;
    m.primitive = primitive;
    m_meshes.push_back(m);
    return (int)(m_meshes.size() - 1u);
}

int Renderer3D::create_material(int ctx, int program, int texture0, unsigned int flags,
                                int uColorLoc, int uParamsLoc,
                                int uBoneTexLoc, int uBoneCountLoc,
                                const char* frameBlockName, int frameBinding) {
    Material m;
    m.program = program;
    m.texture0 = texture0;
    m.flags = flags;
    m.uColorLoc = uColorLoc;
    m.uParamsLoc = uParamsLoc;
    m.uBoneTexLoc = uBoneTexLoc;
    m.uBoneCountLoc = uBoneCountLoc;
    m_materials.push_back(m);
    const int id = (int)(m_materials.size() - 1u);
    if (frameBlockName && frameBinding >= 0) {
        bind_material_ubo(ctx, id, frameBlockName, frameBinding);
    }
    return id;
}

bool Renderer3D::bind_material_ubo(int ctx, int id, const char* blockName, int bindingIndex) {
    Material* mat = nullptr;
    if (id >= 0 && (unsigned int)id < m_materials.size()) mat = &m_materials[(unsigned int)id];
    if (!mat || !blockName) return false;
    const int blockIndex = gl_get_uniform_block_index(ctx, mat->program, ptr_i32(blockName), raw_strlen(blockName));
    if (blockIndex < 0) return false;
    gl_uniform_block_binding(ctx, mat->program, blockIndex, bindingIndex);
    mat->frameUboBinding = bindingIndex;
    return true;
}

bool Renderer3D::set_material_color(int id, const Vec4& color) {
    Material* mat = nullptr;
    if (id >= 0 && (unsigned int)id < m_materials.size()) mat = &m_materials[(unsigned int)id];
    if (!mat) return false;
    mat->color[0] = color.x; mat->color[1] = color.y; mat->color[2] = color.z; mat->color[3] = color.w;
    return true;
}

bool Renderer3D::set_material_params(int id, const Vec4& params) {
    Material* mat = nullptr;
    if (id >= 0 && (unsigned int)id < m_materials.size()) mat = &m_materials[(unsigned int)id];
    if (!mat) return false;
    mat->params[0] = params.x; mat->params[1] = params.y; mat->params[2] = params.z; mat->params[3] = params.w;
    return true;
}

bool Renderer3D::set_material_bones(int id, int boneTex, int boneCount) {
    Material* mat = nullptr;
    if (id >= 0 && (unsigned int)id < m_materials.size()) mat = &m_materials[(unsigned int)id];
    if (!mat) return false;
    mat->boneTex = boneTex;
    mat->boneCount = boneCount;
    return true;
}

bool Renderer3D::update_bone_matrices(int ctx, int materialId, const float* matrices, int boneCount) {
    Material* mat = nullptr;
    if (materialId >= 0 && (unsigned int)materialId < m_materials.size()) mat = &m_materials[(unsigned int)materialId];
    if (!mat || !matrices || boneCount <= 0) return false;
    const int w = 4;
    const int h = boneCount;
    if (mat->boneTex == 0 || mat->boneTexH != h || mat->boneTexW != w) {
        mat->boneTex = create_bone_texture(ctx, w, h);
        mat->boneTexW = w;
        mat->boneTexH = h;
    }
    mat->boneCount = boneCount;
    update_bone_texture(ctx, mat->boneTex, matrices, w, h);
    return true;
}

void Renderer3D::set_frame_matrices(const float* view16, const float* proj16) {
    if (view16) raw_memcpy(m_frame.view, view16, sizeof(m_frame.view));
    if (proj16) raw_memcpy(m_frame.proj, proj16, sizeof(m_frame.proj));
    m_frameDirty = true;
}

void Renderer3D::set_frame_light(const Vec3& dir, const Vec3& color) {
    m_frame.lightDir[0] = dir.x;
    m_frame.lightDir[1] = dir.y;
    m_frame.lightDir[2] = dir.z;
    m_frame.lightDir[3] = 0.0f;
    m_frame.lightColor[0] = color.x;
    m_frame.lightColor[1] = color.y;
    m_frame.lightColor[2] = color.z;
    m_frame.lightColor[3] = 1.0f;
    m_frameDirty = true;
}

int Renderer3D::create_bone_texture(int ctx, int w, int h) {
    return js_tex_create_f32(ctx, w, h);
}

void Renderer3D::update_bone_texture(int ctx, int texId, const float* data, int w, int h) {
    if (!data || texId <= 0 || w <= 0 || h <= 0) return;
    const int count = w * h * 4;
    js_tex_update_f32(ctx, texId, w, h, ptr_i32(data), count);
}

void Renderer3D::ensure_frame_ubo(int ctx, int bindingIndex) {
    if (m_frameUbo == 0) {
        m_frameUbo = gl_create_buffer(ctx);
        m_frameDirty = true;
    }
    m_frameUboBinding = bindingIndex;
}

const Renderer3D::Mesh* Renderer3D::get_mesh(int id) const {
    if (id < 0) return nullptr;
    unsigned int uid = (unsigned int)id;
    if (uid >= m_meshes.size()) return nullptr;
    return &m_meshes[uid];
}

const Renderer3D::Material* Renderer3D::get_material(int id) const {
    if (id < 0) return nullptr;
    unsigned int uid = (unsigned int)id;
    if (uid >= m_materials.size()) return nullptr;
    return &m_materials[uid];
}

void Renderer3D::push_depth(int meshId, int materialId, unsigned int sortKey, unsigned int flags) {
    DrawItem it;
    it.meshId = meshId;
    it.materialId = materialId;
    it.mesh = get_mesh(meshId);
    it.material = get_material(materialId);
    it.sortKey = sortKey;
    it.flags = flags;
    m_depth.push_back(it);
}

void Renderer3D::push_opaque(int meshId, int materialId, unsigned int sortKey, unsigned int flags) {
    DrawItem it;
    it.meshId = meshId;
    it.materialId = materialId;
    it.mesh = get_mesh(meshId);
    it.material = get_material(materialId);
    it.sortKey = sortKey;
    it.flags = flags;
    m_opaque.push_back(it);
}

void Renderer3D::push_transparent(int meshId, int materialId, unsigned int sortKey, unsigned int flags) {
    DrawItem it;
    it.meshId = meshId;
    it.materialId = materialId;
    it.mesh = get_mesh(meshId);
    it.material = get_material(materialId);
    it.sortKey = sortKey;
    it.flags = flags;
    m_transparent.push_back(it);
}

void Renderer3D::push_emissive(int meshId, int materialId, unsigned int sortKey, unsigned int flags) {
    DrawItem it;
    it.meshId = meshId;
    it.materialId = materialId;
    it.mesh = get_mesh(meshId);
    it.material = get_material(materialId);
    it.sortKey = sortKey;
    it.flags = flags;
    m_emissive.push_back(it);
}

static void sort_draw_items(shine::wasm::SVector<Renderer3D::DrawItem>& list, bool descending) {
    if (list.size() < 2u) return;
    struct DrawItemLess {
        bool desc = false;
        bool operator()(const Renderer3D::DrawItem& a, const Renderer3D::DrawItem& b) const {
            return desc ? (a.sortKey > b.sortKey) : (a.sortKey < b.sortKey);
        }
    };
    shine::util::sort_inplace(list.data(), list.size(), DrawItemLess{descending});
}

static inline unsigned long long hash_step(unsigned long long h, unsigned int v) {
    return (h ^ (unsigned long long)v) * 1099511628211ull;
}

static inline unsigned long long hash_material(const Renderer3D::Material& mat) {
    unsigned long long h = 1469598103934665603ull;
    h = hash_step(h, (unsigned int)mat.program);
    h = hash_step(h, (unsigned int)mat.texture0);
    h = hash_step(h, (unsigned int)mat.uColorLoc);
    h = hash_step(h, (unsigned int)mat.uParamsLoc);
    h = hash_step(h, (unsigned int)mat.uBoneTexLoc);
    h = hash_step(h, (unsigned int)mat.uBoneCountLoc);
    h = hash_step(h, (unsigned int)mat.boneTex);
    h = hash_step(h, (unsigned int)mat.boneCount);
    h = hash_step(h, (unsigned int)f2i(mat.color[0]));
    h = hash_step(h, (unsigned int)f2i(mat.color[1]));
    h = hash_step(h, (unsigned int)f2i(mat.color[2]));
    h = hash_step(h, (unsigned int)f2i(mat.color[3]));
    h = hash_step(h, (unsigned int)f2i(mat.params[0]));
    h = hash_step(h, (unsigned int)f2i(mat.params[1]));
    h = hash_step(h, (unsigned int)f2i(mat.params[2]));
    h = hash_step(h, (unsigned int)f2i(mat.params[3]));
    return h;
}

static void apply_material(const Renderer3D::FrameUniforms& frame, const Renderer3D::Material& mat,
                           int& curProg, int& curTex, int& curActiveTex,
                           const Renderer3D::Material*& lastMat,
                           unsigned long long& lastHash,
                           shine::graphics::CommandBuffer::Pass& pass) {
    const unsigned long long h = hash_material(mat);
    if (lastMat == &mat || lastHash == h) {
        lastMat = &mat;
        lastHash = h;
        return;
    }
    lastMat = &mat;
    lastHash = h;
    if (mat.program != curProg) {
        curProg = mat.program;
        pass.push(shine::graphics::CMD_USE_PROGRAM, curProg, 0, 0, 0, 0, 0, 0);
    }

    if (mat.boneTex != 0) {
        if (curActiveTex != 1) {
            pass.push(shine::graphics::CMD_ACTIVE_TEXTURE, 1, 0, 0, 0, 0, 0, 0);
            curActiveTex = 1;
        }
        if (mat.boneTex != curTex) {
            curTex = mat.boneTex;
            pass.push(shine::graphics::CMD_BIND_TEXTURE, GL_TEXTURE_2D, curTex, 0, 0, 0, 0, 0);
        }
        if (mat.uBoneTexLoc >= 0) {
            pass.push(shine::graphics::CMD_UNIFORM1I, mat.uBoneTexLoc, 1, 0, 0, 0, 0, 0);
        }
    }

    if (curActiveTex != 0) {
        pass.push(shine::graphics::CMD_ACTIVE_TEXTURE, 0, 0, 0, 0, 0, 0, 0);
        curActiveTex = 0;
    }
    if (mat.texture0 != curTex) {
        curTex = mat.texture0;
        if (curTex != 0) pass.push(shine::graphics::CMD_BIND_TEXTURE, GL_TEXTURE_2D, curTex, 0, 0, 0, 0, 0);
    }
    if (mat.uColorLoc >= 0) {
        pass.push(shine::graphics::CMD_UNIFORM4F, mat.uColorLoc,
        f2i(mat.color[0]), f2i(mat.color[1]),
        f2i(mat.color[2]), f2i(mat.color[3]), 0, 0);
    }
    if (mat.uParamsLoc >= 0) {
        pass.push(shine::graphics::CMD_UNIFORM4F, mat.uParamsLoc,
        f2i(mat.params[0]), f2i(mat.params[1]),
        f2i(mat.params[2]), f2i(mat.params[3]), 0, 0);
    }
    (void)frame;
    if (mat.uBoneTexLoc >= 0) {
        pass.push(shine::graphics::CMD_UNIFORM1I, mat.uBoneTexLoc, mat.boneTex ? 1 : 0, 0, 0, 0, 0, 0);
    }
    if (mat.uBoneCountLoc >= 0) {
        pass.push(shine::graphics::CMD_UNIFORM1I, mat.uBoneCountLoc, mat.boneCount, 0, 0, 0, 0, 0);
    }
}

void Renderer3D::push_frame_ubo(shine::graphics::CommandBuffer::Pass& pass) {
    if (m_frameUbo == 0) return;
    pass.push(shine::graphics::CMD_BIND_BUFFER, GL_UNIFORM_BUFFER, m_frameUbo, 0, 0, 0, 0, 0);
    if (m_frameDirty) {
        for (int i = 0; i < 16; ++i) m_framePacked[i] = m_frame.view[i];
        for (int i = 0; i < 16; ++i) m_framePacked[16 + i] = m_frame.proj[i];
        for (int i = 0; i < 4; ++i) m_framePacked[32 + i] = m_frame.lightDir[i];
        for (int i = 0; i < 4; ++i) m_framePacked[36 + i] = m_frame.lightColor[i];
        pass.push(shine::graphics::CMD_BUFFER_DATA_F32, GL_UNIFORM_BUFFER, ptr_i32(m_framePacked), 40, GL_DYNAMIC_DRAW, 0, 0, 0);
        m_frameDirty = false;
    }
    pass.push(shine::graphics::CMD_BIND_BUFFER_BASE, GL_UNIFORM_BUFFER, m_frameUboBinding, m_frameUbo, 0, 0, 0, 0);
}

void Renderer3D::flush_list(shine::wasm::SVector<DrawItem>& list, bool descending, shine::graphics::CommandBuffer::Pass& pass) {
    sort_draw_items(list, descending);
    int curProg = -1;
    int curVao = -1;
    int curTex = -1;
    int curActiveTex = 0;
    const Material* lastMat = nullptr;
    unsigned long long lastHash = 0ull;
    push_frame_ubo(pass);
    for (unsigned int i = 0; i < list.size(); ++i) {
        const DrawItem& it = list[i];
        const Mesh* mesh = it.mesh ? it.mesh : get_mesh(it.meshId);
        const Material* mat = it.material ? it.material : get_material(it.materialId);
        if (!mesh || !mat || mesh->vertexCount <= 0) continue;

        apply_material(m_frame, *mat, curProg, curTex, curActiveTex, lastMat, lastHash, pass);
        if (mesh->vao != curVao) {
            curVao = mesh->vao;
            pass.push(shine::graphics::CMD_BIND_VAO, curVao, 0, 0, 0, 0, 0, 0);
        }
        pass.push(shine::graphics::CMD_DRAW_ARRAYS, mesh->primitive, 0, mesh->vertexCount, 0, 0, 0, 0);
    }
}

void Renderer3D::flush_depth(shine::graphics::CommandBuffer::Pass& pass) {
    flush_list(m_depth, false, pass);
    m_depth.clear();
}

void Renderer3D::flush_opaque(shine::graphics::CommandBuffer::Pass& pass) {
    flush_list(m_opaque, false, pass);
    m_opaque.clear();
}

void Renderer3D::flush_transparent(shine::graphics::CommandBuffer::Pass& pass) {
    flush_list(m_transparent, true, pass);
    m_transparent.clear();
}

void Renderer3D::flush_emissive(shine::graphics::CommandBuffer::Pass& pass) {
    flush_list(m_emissive, false, pass);
    m_emissive.clear();
}

} // namespace renderer
} // namespace shine
