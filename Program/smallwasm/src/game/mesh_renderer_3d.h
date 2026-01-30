#ifndef SHINE_GAME_MESH_RENDERER_3D_H
#define SHINE_GAME_MESH_RENDERER_3D_H

#include "transform.h"
#include "../renderer/renderer_3d.h"

namespace shine {
namespace game {

class MeshRenderer3D final : public Component {
public:
    int meshId = -1;
    int materialId = -1;
    int transparent = 0;
    int depthOnly = 0;
    int emissive = 0;

    inline void setTransparent(bool v) noexcept { transparent = v ? 1 : 0; }
    inline void setDepthOnly(bool v) noexcept { depthOnly = v ? 1 : 0; }
    inline void setEmissive(bool v) noexcept { emissive = v ? 1 : 0; }

    void onRender(RenderContext& rc, float /*t*/) override {
        if (!node || meshId < 0 || materialId < 0) return;
        if (rc.pass == renderer::PASS_DEPTH) {
            if (!depthOnly) return;
        } else if (rc.pass == renderer::PASS_OPAQUE) {
            if (transparent || emissive) return;
        } else if (rc.pass == renderer::PASS_TRANSPARENT) {
            if (!transparent) return;
        } else if (rc.pass == renderer::PASS_EMISSIVE) {
            if (!emissive) return;
        } else {
            return;
        }

        Transform* tr = node->getComponent<Transform>();
        if (!tr) return;
        float cx, cy, cz;
        tr->worldXYZ(cx, cy, cz);

        // SortKey layout: [layer:8][material:8][depth:16]
        unsigned int layer = rc.sortKey & 0xFFu;
        unsigned int mat = (unsigned int)(materialId & 0xFF);
        float nz = (cz + 1.0f) * 0.5f;
        if (nz < 0.0f) nz = 0.0f;
        if (nz > 1.0f) nz = 1.0f;
        unsigned int depthKey = (unsigned int)(nz * 65535.0f);
        if (transparent) depthKey = 65535u - depthKey;
        unsigned int sortKey = (layer << 24) | (mat << 16) | depthKey;

        renderer::Renderer3D& r = renderer::Renderer3D::instance();
        if (rc.pass == renderer::PASS_DEPTH) r.push_depth(meshId, materialId, sortKey);
        else if (rc.pass == renderer::PASS_OPAQUE) r.push_opaque(meshId, materialId, sortKey);
        else if (rc.pass == renderer::PASS_TRANSPARENT) r.push_transparent(meshId, materialId, sortKey);
        else if (rc.pass == renderer::PASS_EMISSIVE) r.push_emissive(meshId, materialId, sortKey);
    }
};

} // namespace game
} // namespace shine

#endif // SHINE_GAME_MESH_RENDERER_3D_H
