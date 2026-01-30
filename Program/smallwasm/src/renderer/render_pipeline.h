#pragma once

namespace shine {
namespace renderer {

// Typical passes for a lightweight wasm renderer.
enum RenderPassType : unsigned char {
    PASS_SHADOW = 0,
    PASS_DEPTH = 1,
    PASS_OPAQUE = 2,
    PASS_TRANSPARENT = 3,
    PASS_BASE_COLOR = 4,
    PASS_UNLIT = 5,
    PASS_EMISSIVE = 6,
    PASS_UI = 7,
    PASS_POST_PROCESS = 8,
    PASS_COUNT = 9,
};

struct RenderPass {
    RenderPassType type = PASS_BASE_COLOR;
    unsigned int sortKey = 0;
    unsigned int flags = 0;
};

class RenderPipeline {
public:
    static constexpr unsigned int MAX_PASSES = 16;

    void reset() { m_count = 0; }

    bool add_pass(RenderPassType type, unsigned int sortKey = 0, unsigned int flags = 0) {
        if (m_count >= MAX_PASSES) return false;
        RenderPass& p = m_passes[m_count++];
        p.type = type;
        p.sortKey = sortKey;
        p.flags = flags;
        return true;
    }

    bool set_pass(RenderPassType type, unsigned int sortKey, unsigned int flags = 0) {
        for (unsigned int i = 0; i < m_count; ++i) {
            if (m_passes[i].type == type) {
                m_passes[i].sortKey = sortKey;
                m_passes[i].flags = flags;
                return true;
            }
        }
        return false;
    }

    void add_default_passes() {
        reset();
        add_pass(PASS_SHADOW);
        add_pass(PASS_DEPTH);
        add_pass(PASS_OPAQUE);
        add_pass(PASS_TRANSPARENT);
        add_pass(PASS_BASE_COLOR);
        add_pass(PASS_UNLIT);
        add_pass(PASS_EMISSIVE);
        add_pass(PASS_UI);
        add_pass(PASS_POST_PROCESS);
    }

    const RenderPass* data() const { return m_passes; }
    unsigned int count() const { return m_count; }

private:
    RenderPass m_passes[MAX_PASSES];
    unsigned int m_count = 0;
};

} // namespace renderer
} // namespace shine
