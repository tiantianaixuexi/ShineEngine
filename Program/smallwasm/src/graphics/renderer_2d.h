#pragma once

#include "../Container/SVector.h" // Fixed include path case sensitivity?
#include "../math/Rect.h"
#include "../math/Color4.h"
#include "wasm_command_buffer.h"

namespace shine::graphics {

// Renderer2D: High-level 2D primitives drawing.
// Translates 2D commands into command buffer operations.
class Renderer2D {
public:
    static Renderer2D& instance();

    // Init programs, etc.
    void init(int ctx);

    // Primitives
    void drawRectColor(float cx, float cy, float w, float h, float r, float g, float b);
    void drawRectUV(int texId, float cx, float cy, float w, float h);
    void drawRectColorSorted(float cx, float cy, float w, float h, float r, float g, float b, unsigned int sortKey);
    void drawRectUVSorted(int texId, float cx, float cy, float w, float h, unsigned int sortKey);
    using Rect = shine::math::Rect;
    using Color4 = shine::math::Color4;
    struct RoundRectStyle {
        float radius_px = 0.0f;
        Color4 fill{};
        int texId = 0;
        Color4 texTint{1.0f, 1.0f, 1.0f, 1.0f};
        float border_px = 0.0f;
        Color4 borderColor{};
        float shadow_off_x = 0.0f;
        float shadow_off_y = 0.0f;
        float shadow_blur = 0.0f;
        float shadow_spread = 0.0f;
        Color4 shadowColor{};
    };

    void drawRoundRect(const Rect& rect, const RoundRectStyle& style);

    inline void drawRectColor(const Rect& rect, const Color4& color) {
        drawRectColor(rect.cx, rect.cy, rect.w, rect.h, color.r, color.g, color.b);
    }
    inline void drawRectUV(int texId, const Rect& rect) {
        drawRectUV(texId, rect.cx, rect.cy, rect.w, rect.h);
    }
    inline void drawRectColorSorted(const Rect& rect, const Color4& color, unsigned int sortKey) {
        drawRectColorSorted(rect.cx, rect.cy, rect.w, rect.h, color.r, color.g, color.b, sortKey);
    }
    inline void drawRectUVSorted(int texId, const Rect& rect, unsigned int sortKey) {
        drawRectUVSorted(texId, rect.cx, rect.cy, rect.w, rect.h, sortKey);
    }

    // Frame management
    void begin(); // Called at start of frame
    void end();   // Called at end of frame to submit batches
    void flush(); // Internal or forced flush

    float* allocVtx(int floatCount, int* out_first_vertex = nullptr); 
    
    // State
    int m_ctx = 0;
    int m_vbo = 0;
    int m_vao = 0; // Added VAO
    
    // Shader - Tex
    int m_prog_tex = 0;
    int m_uTex = 0;
    int m_uViewSize = 0; // For Tex shader

    int m_prog_col = 0;
    int m_uCol_ViewSize = 0;

    int m_prog_rr = 0;
    int m_uRR_ViewSize = 0; // For RR shader
    int m_uRR_Tex = 0;
    int m_uRR_UseTex = 0;
    int m_uRR_Color = 0;
    int m_uRR_Rad = 0;
    int m_uRR_TexTint = 0;
    int m_uRR_BorderColor = 0;
    int m_uRR_Border = 0;
    int m_uRR_ShadowColor = 0;
    int m_uRR_ShadowOff = 0;
    int m_uRR_ShadowBlur = 0;
    int m_uRR_ShadowSpread = 0;

    // Viewport info for pixel-size calculations (e.g. rounded rect radius)
    int m_viewW = 0;
    int m_viewH = 0;
    int m_lastUploadCount = 0;

    // Make constructor public for simplicity with static instance
    Renderer2D() = default;

private:
    struct RRUniformState {
        int useTex = 0;
        int radX = 0;
        int radY = 0;

        int colorR = 0;
        int colorG = 0;
        int colorB = 0;
        int colorA = 0;

        int texTintR = 0;
        int texTintG = 0;
        int texTintB = 0;
        int texTintA = 0;

        int borderColorR = 0;
        int borderColorG = 0;
        int borderColorB = 0;
        int borderColorA = 0;

        int border = 0;

        int shadowColorR = 0;
        int shadowColorG = 0;
        int shadowColorB = 0;
        int shadowColorA = 0;

        int shadowOffX = 0;
        int shadowOffY = 0;

        int shadowBlur = 0;
        int shadowSpread = 0;
    };

    struct Batch {
        int texId = 0;
        int offset = 0; // vertex count offset
        int count = 0;  // vertex count
        unsigned int sortKey = 0;
        // We could store shader type if we mix shaders, but now we use RR shader for everything?
        // Or switch shaders. For now let's assume we might switch.
        // But to keep it simple, let's try to use RR shader for everything if possible, 
        // OR handle shader switch.
        // Current impl has drawRectUV (Tex shader) and drawRoundRect (RR shader).
        // Let's store a "mode" or "shaderId".
        int shaderId = 0; // 0=Tex, 1=RR, 2=Col
        RRUniformState rr;
    };

    // checkBatch and checkBatchRR inlined manually in draw functions to reduce call overhead
    void checkBatch(int shaderId, int texId, int firstVertex, int numVerts, unsigned int sortKey);
    void checkBatchRR(int texId, const RRUniformState& rr, int firstVertex, int numVerts, unsigned int sortKey);
    void updateRRUniforms(const RRUniformState& b, RRUniformState& last, bool& hasLastRR, CommandBuffer::Pass& pass);

    // Per-frame UI vertex buffer stream
    shine::wasm::SVector<float> m_ui_vtx;
    shine::wasm::SVector<Batch> m_batches;
    shine::wasm::SVector<int> m_rr_blocks;
    bool m_need_sort = false;
};

} // namespace shine::graphics

// Global helper macro for easy access
#define RENDERER_2D (shine::graphics::Renderer2D::instance())
