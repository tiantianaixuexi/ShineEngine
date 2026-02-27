#pragma once

#include "../Container/SVector.h"
#include "../graphics/wasm_command_buffer.h"
#include "../graphics/gl_api.h"
#include "../math/Vector3.h"
#include "../math/Vector4.h"
#include "render_pipeline.h"

namespace shine {
namespace renderer {

using shine::math::Vec3;
using shine::math::Vec4;

// Minimal 3D renderer skeleton with depth/opaque/transparent queues.
class Renderer3D {
public:
    struct Mesh {
        int vao = 0;
        int vertexCount = 0;
        int primitive = GL_TRIANGLES;
    };

    struct Material {
        int program = 0;
        int texture0 = 0;
        unsigned int flags = 0;
        int uColorLoc = -1;
        int uParamsLoc = -1;
        int uBoneTexLoc = -1;
        int uBoneCountLoc = -1;
        float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        float params[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        int boneTex = 0;
        int boneCount = 0;
        int boneTexW = 0;
        int boneTexH = 0;
        int frameUboBinding = -1;
    };

    struct FrameUniforms {
        float view[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        float proj[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        float lightDir[4] = {0.0f, 1.0f, 0.0f, 0.0f};
        float lightColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    };

    struct DrawItem {
        int meshId = 0;
        int materialId = 0;
        const Mesh* mesh = nullptr;
        const Material* material = nullptr;
        unsigned int sortKey = 0;
        unsigned int flags = 0;
    };

    static Renderer3D& instance();
    Renderer3D() = default;

    void reset();
    void begin_frame();

    int create_mesh(int vao, int vertexCount, int primitive = GL_TRIANGLES);
    int create_material(int ctx, int program, int texture0 = 0, unsigned int flags = 0,
                        int uColorLoc = -1, int uParamsLoc = -1,
                        int uBoneTexLoc = -1, int uBoneCountLoc = -1,
                        const char* frameBlockName = nullptr, int frameBinding = -1);
    bool bind_material_ubo(int ctx, int id, const char* blockName, int bindingIndex);
    bool set_material_color(int id, const Vec4& color);
    bool set_material_params(int id, const Vec4& params);
    bool set_material_bones(int id, int boneTex, int boneCount);
    bool update_bone_matrices(int ctx, int materialId, const float* matrices, int boneCount);

    void set_frame_matrices(const float* view16, const float* proj16);
    void set_frame_light(const Vec3& dir, const Vec3& color);
    int create_bone_texture(int ctx, int w, int h);
    void update_bone_texture(int ctx, int texId, const float* data, int w, int h);
    void ensure_frame_ubo(int ctx, int bindingIndex);

    void push_depth(int meshId, int materialId, unsigned int sortKey = 0, unsigned int flags = 0);
    void push_opaque(int meshId, int materialId, unsigned int sortKey = 0, unsigned int flags = 0);
    void push_transparent(int meshId, int materialId, unsigned int sortKey = 0, unsigned int flags = 0);
    void push_emissive(int meshId, int materialId, unsigned int sortKey = 0, unsigned int flags = 0);

    inline unsigned int depth_count() const noexcept { return m_depth.size(); }
    inline unsigned int opaque_count() const noexcept { return m_opaque.size(); }
    inline unsigned int transparent_count() const noexcept { return m_transparent.size(); }
    inline unsigned int emissive_count() const noexcept { return m_emissive.size(); }

    void flush_depth(shine::graphics::CommandBuffer::Pass& pass);
    void flush_opaque(shine::graphics::CommandBuffer::Pass& pass);
    void flush_transparent(shine::graphics::CommandBuffer::Pass& pass);
    void flush_emissive(shine::graphics::CommandBuffer::Pass& pass);

private:
    const Mesh* get_mesh(int id) const;
    const Material* get_material(int id) const;
    void push_frame_ubo(shine::graphics::CommandBuffer::Pass& pass);
    void flush_list(shine::wasm::SVector<DrawItem>& list, bool descending, shine::graphics::CommandBuffer::Pass& pass);

    shine::wasm::SVector<Mesh> m_meshes;
    shine::wasm::SVector<Material> m_materials;
    shine::wasm::SVector<DrawItem> m_depth;
    shine::wasm::SVector<DrawItem> m_opaque;
    shine::wasm::SVector<DrawItem> m_transparent;
    shine::wasm::SVector<DrawItem> m_emissive;
    FrameUniforms m_frame;
    float m_framePacked[40] = {};
    int m_frameUbo = 0;
    int m_frameUboBinding = 0;
    bool m_frameDirty = true;
};

} // namespace renderer
} // namespace shine
