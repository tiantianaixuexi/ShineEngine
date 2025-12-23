#pragma once

#include "../Container/SVector.h" // Fixed include path case sensitivity?

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
    void drawRoundRect(float cx, float cy, float w, float h,
                       float radius_px,
                       float fill_r, float fill_g, float fill_b, float fill_a,
                       int texId,
                       float texTint_r, float texTint_g, float texTint_b, float texTint_a,
                       float border_px,
                       float border_r, float border_g, float border_b, float border_a,
                       float shadow_off_px_x, float shadow_off_px_y,
                       float shadow_blur_px, float shadow_spread_px,
                       float shadow_r, float shadow_g, float shadow_b, float shadow_a);

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

        inline bool operator==(const RRUniformState& o) const noexcept {
            return useTex == o.useTex &&
                   radX == o.radX && radY == o.radY &&
                   colorR == o.colorR && colorG == o.colorG && colorB == o.colorB && colorA == o.colorA &&
                   texTintR == o.texTintR && texTintG == o.texTintG && texTintB == o.texTintB && texTintA == o.texTintA &&
                   borderColorR == o.borderColorR && borderColorG == o.borderColorG && borderColorB == o.borderColorB && borderColorA == o.borderColorA &&
                   border == o.border &&
                   shadowColorR == o.shadowColorR && shadowColorG == o.shadowColorG && shadowColorB == o.shadowColorB && shadowColorA == o.shadowColorA &&
                   shadowOffX == o.shadowOffX && shadowOffY == o.shadowOffY &&
                   shadowBlur == o.shadowBlur && shadowSpread == o.shadowSpread;
        }
    };

    struct Batch {
        int texId = 0;
        int offset = 0; // vertex count offset
        int count = 0;  // vertex count
        // We could store shader type if we mix shaders, but now we use RR shader for everything?
        // Or switch shaders. For now let's assume we might switch.
        // But to keep it simple, let's try to use RR shader for everything if possible, 
        // OR handle shader switch.
        // Current impl has drawRectUV (Tex shader) and drawRoundRect (RR shader).
        // Let's store a "mode" or "shaderId".
        int shaderId = 0; // 0=Tex, 1=RR, 2=Col
        RRUniformState rr;
    };

    void checkBatch(int shaderId, int texId, int firstVertex, int numVerts);
    void checkBatchRR(int texId, const RRUniformState& rr, int firstVertex, int numVerts);

    // Per-frame UI vertex buffer stream
    shine::wasm::SVector<float> m_ui_vtx;
    shine::wasm::SVector<Batch> m_batches;
};

} // namespace shine::graphics

// Global helper macro for easy access
#define RENDERER_2D (shine::graphics::Renderer2D::instance())
