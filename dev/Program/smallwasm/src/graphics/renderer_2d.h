#pragma once

#include "../Container/SVector.h"
#include "../math/Rect.h"
#include "../math/Color4.h"
#include "wasm_command_buffer.h"

namespace shine::graphics {

class Renderer2D {
public:
    static Renderer2D& instance();

    void init(int ctx);

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

    void begin();
    void end();
    void flush();
    float* allocVtx(int floatCount, int* out_first_vertex = nullptr);

    // State
    int m_ctx = 0;
    int m_vbo = 0;
    int m_vao = 0;
    
    int m_prog_tex = 0;
    int m_uTex = 0;
    int m_uViewSize = 0;

    int m_prog_col = 0;
    int m_uCol_ViewSize = 0;

    int m_prog_rr = 0;
    int m_uRR_ViewSize = 0;
    int m_uRR_Tex = 0;

    // 存储 RR 相关的 uniform 位置，共 10 个
    int m_rrUniformLocs[10] = {0};

    int m_viewW = 0;
    int m_viewH = 0;
    int m_lastUploadCount = 0;

    Renderer2D() = default;


    public:
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
        int offset = 0;
        int count = 0;
        unsigned int sortKey = 0;
        int shaderId = 0;
        int rrIndex = -1;
    };


    private:
    void checkBatch(int shaderId, int texId, int firstVertex, int numVerts, unsigned int sortKey);
    void checkBatchRR(int texId, const RRUniformState& rr, int firstVertex, int numVerts, unsigned int sortKey);
    void updateRRUniforms(const RRUniformState& b, RRUniformState& last, bool& hasLastRR, CommandBuffer::Pass& pass);
    void switchShader(const Batch& b, int& curShaderId, unsigned int& setupMask, CommandBuffer::Pass& pass);

    shine::wasm::SVector<float> m_ui_vtx;
    shine::wasm::SVector<Batch> m_batches;
    shine::wasm::SVector<int> m_rr_blocks;
    shine::wasm::SVector<RRUniformState> m_rr_states;
    bool m_need_sort = false;
};

} // namespace shine::graphics

#define RENDERER_2D (shine::graphics::Renderer2D::instance())