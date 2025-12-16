#include "renderer_2d.h"
#include "gl_api.h"
#include "../util/wasm_compat.h"

namespace shine {
namespace graphics {

// Shader strings
static const char kVS_TEX[] =
  "attribute vec2 aPos;\n"
  "attribute vec3 aCol;\n"
  "varying vec2 vUV;\n"
  "uniform vec2 uViewSize;\n"
  "void main(){\n"
  "  vUV = aCol.xy;\n"
  "  vec2 nPos = (aPos / uViewSize) * 2.0 - 1.0;\n"
  "  gl_Position = vec4(nPos.x, -nPos.y, 0.0, 1.0);\n" // Y flip for top-left 0,0
  "}\n";

static const char kFS_TEX[] =
  "precision mediump float;\n"
  "varying vec2 vUV;\n"
  "uniform sampler2D uTex;\n"
  "void main(){ gl_FragColor = texture2D(uTex, vUV); }\n";

static const char kU_TEX[] = "uTex";
static const char kU_VIEW[] = "uViewSize";

static const char kVS_RR[] =
  "attribute vec2 aPos;\n"
  "attribute vec3 aCol;\n"
  "varying vec2 vUV;\n"
  "uniform vec2 uViewSize;\n"
  "void main(){\n"
  "  vUV = aCol.xy;\n"
  "  vec2 nPos = (aPos / uViewSize) * 2.0 - 1.0;\n"
  "  gl_Position = vec4(nPos.x, -nPos.y, 0.0, 1.0);\n"
  "}\n";

static const char kFS_RR[] =
  "#ifdef GL_OES_standard_derivatives\n"
  "#extension GL_OES_standard_derivatives : enable\n"
  "#endif\n"
  "precision mediump float;\n"
  "varying vec2 vUV;\n"
  "uniform vec4 uColor;\n"
  "uniform vec4 uTexTint;\n"
  "uniform vec4 uBorderColor;\n"
  "uniform float uBorder;\n"
  "uniform vec4 uShadowColor;\n"
  "uniform vec2 uShadowOff;\n"
  "uniform float uShadowBlur;\n"
  "uniform float uShadowSpread;\n"
  "uniform vec2 uRad;\n"
  "uniform int uUseTex;\n"
  "uniform sampler2D uTex;\n"
  "float sdfRoundRect(vec2 uv, vec2 rad){\n"
  "  vec2 p = uv - vec2(0.5, 0.5);\n"
  "  vec2 halfSize = vec2(0.5, 0.5);\n"
  "  vec2 q = abs(p) - (halfSize - rad);\n"
  "  vec2 mq = max(q, vec2(0.0, 0.0));\n"
  "  float outside = length(mq);\n"
  "  float inside = min(max(q.x, q.y), 0.0);\n"
  "  return outside + inside - min(rad.x, rad.y);\n"
  "}\n"
  "void main(){\n"
  "  vec2 rad = clamp(uRad, vec2(0.0,0.0), vec2(0.5,0.5));\n"
  "  float d = sdfRoundRect(vUV, rad);\n"
  "  float aa = 1.0/256.0;\n"
  "#ifdef GL_OES_standard_derivatives\n"
  "  aa = fwidth(d);\n"
  "#endif\n"
  "  float fill = 1.0 - smoothstep(0.0, aa, d);\n"
  "  float t = max(0.0, uBorder);\n"
  "  float inner = 1.0 - smoothstep(-t, -t + aa, d);\n"
  "  float border = clamp(fill - inner, 0.0, 1.0);\n"
  "  vec4 base = uColor;\n"
  "  if(uUseTex!=0){ base *= texture2D(uTex, vUV) * uTexTint; }\n"
  "  vec4 cFill = vec4(base.rgb, base.a * fill);\n"
  "  vec4 cBorder = vec4(uBorderColor.rgb, uBorderColor.a * border);\n"
  "  float ds = sdfRoundRect(vUV - uShadowOff, rad) - uShadowSpread;\n"
  "  float sb = max(0.0, uShadowBlur);\n"
  "  float shadow = 1.0 - smoothstep(0.0, sb + aa, ds);\n"
  "  vec4 cShadow = vec4(uShadowColor.rgb, uShadowColor.a * shadow);\n"
  "  vec4 outc = cShadow;\n"
  "  outc = outc + cBorder * (1.0 - outc.a);\n"
  "  outc = outc + cFill * (1.0 - outc.a);\n"
  "  gl_FragColor = outc;\n"
  "}\n";

static const char kU_RR_TEX[] = "uTex";
static const char kU_RR_USETEX[] = "uUseTex";
static const char kU_RR_COLOR[] = "uColor";
static const char kU_RR_RAD[] = "uRad";
static const char kU_RR_TEXTINT[] = "uTexTint";
static const char kU_RR_BORDERCOLOR[] = "uBorderColor";
static const char kU_RR_BORDER[] = "uBorder";
static const char kU_RR_SHADOWCOLOR[] = "uShadowColor";
static const char kU_RR_SHADOWOFF[] = "uShadowOff";
static const char kU_RR_SHADOWBLUR[] = "uShadowBlur";
static const char kU_RR_SHADOWSPREAD[] = "uShadowSpread";

// Safe static instance
static Renderer2D s_renderer;

Renderer2D& Renderer2D::instance() {
    return s_renderer;
}

void Renderer2D::init(int ctx) {
    m_ctx = ctx;
    // VBO & VAO
    m_vbo = gl_create_buffer(ctx);
    m_vao = gl_create_vertex_array(ctx);
    
    gl_bind_vertex_array(ctx, m_vao);
    gl_bind_buffer(ctx, GL_ARRAY_BUFFER, m_vbo);
    gl_enable_attribs(ctx); // Enables aPos (0) and aCol (1) with stride 5 floats
    gl_bind_vertex_array(ctx, 0); // Unbind to be safe

    // Pre-alloc vtx buffer
    m_ui_vtx.reserve(65536);

    // Tex shader
    int vs = gl_create_shader(ctx, GL_VERTEX_SHADER, shine::wasm::ptr_i32(kVS_TEX), sizeof(kVS_TEX)-1);
    int fs = gl_create_shader(ctx, GL_FRAGMENT_SHADER, shine::wasm::ptr_i32(kFS_TEX), sizeof(kFS_TEX)-1);
    m_prog_tex = gl_create_program(ctx, vs, fs);
    m_uTex = gl_get_uniform_location(ctx, m_prog_tex, shine::wasm::ptr_i32(kU_TEX), sizeof(kU_TEX)-1);
    m_uViewSize = gl_get_uniform_location(ctx, m_prog_tex, shine::wasm::ptr_i32(kU_VIEW), sizeof(kU_VIEW)-1);

    // RR shader
    int vs2 = gl_create_shader(ctx, GL_VERTEX_SHADER, shine::wasm::ptr_i32(kVS_RR), sizeof(kVS_RR)-1);
    int fs2 = gl_create_shader(ctx, GL_FRAGMENT_SHADER, shine::wasm::ptr_i32(kFS_RR), sizeof(kFS_RR)-1);
    m_prog_rr = gl_create_program(ctx, vs2, fs2);
    m_uRR_ViewSize = gl_get_uniform_location(ctx, m_prog_rr, shine::wasm::ptr_i32(kU_VIEW), sizeof(kU_VIEW)-1);
    m_uRR_Tex = gl_get_uniform_location(ctx, m_prog_rr, shine::wasm::ptr_i32(kU_RR_TEX), sizeof(kU_RR_TEX)-1);
    m_uRR_UseTex = gl_get_uniform_location(ctx, m_prog_rr, shine::wasm::ptr_i32(kU_RR_USETEX), sizeof(kU_RR_USETEX)-1);
    m_uRR_Color = gl_get_uniform_location(ctx, m_prog_rr, shine::wasm::ptr_i32(kU_RR_COLOR), sizeof(kU_RR_COLOR)-1);
    m_uRR_Rad = gl_get_uniform_location(ctx, m_prog_rr, shine::wasm::ptr_i32(kU_RR_RAD), sizeof(kU_RR_RAD)-1);
    m_uRR_TexTint = gl_get_uniform_location(ctx, m_prog_rr, shine::wasm::ptr_i32(kU_RR_TEXTINT), sizeof(kU_RR_TEXTINT)-1);
    m_uRR_BorderColor = gl_get_uniform_location(ctx, m_prog_rr, shine::wasm::ptr_i32(kU_RR_BORDERCOLOR), sizeof(kU_RR_BORDERCOLOR)-1);
    m_uRR_Border = gl_get_uniform_location(ctx, m_prog_rr, shine::wasm::ptr_i32(kU_RR_BORDER), sizeof(kU_RR_BORDER)-1);
    m_uRR_ShadowColor = gl_get_uniform_location(ctx, m_prog_rr, shine::wasm::ptr_i32(kU_RR_SHADOWCOLOR), sizeof(kU_RR_SHADOWCOLOR)-1);
    m_uRR_ShadowOff = gl_get_uniform_location(ctx, m_prog_rr, shine::wasm::ptr_i32(kU_RR_SHADOWOFF), sizeof(kU_RR_SHADOWOFF)-1);
    m_uRR_ShadowBlur = gl_get_uniform_location(ctx, m_prog_rr, shine::wasm::ptr_i32(kU_RR_SHADOWBLUR), sizeof(kU_RR_SHADOWBLUR)-1);
    m_uRR_ShadowSpread = gl_get_uniform_location(ctx, m_prog_rr, shine::wasm::ptr_i32(kU_RR_SHADOWSPREAD), sizeof(kU_RR_SHADOWSPREAD)-1);

    // Initial Buffer allocation to prevent overflow
    gl_bind_buffer(ctx, GL_ARRAY_BUFFER, m_vbo);
    // Allocate 1MB (256k floats)
    gl_buffer_data_f32(ctx, GL_ARRAY_BUFFER, 0, 256 * 1024, GL_DYNAMIC_DRAW); 
}

void Renderer2D::begin() {
    m_ui_vtx.clear();
    m_batches.clear();
    // Pre-allocate to avoid pointer invalidation and memory movement during frame
    m_ui_vtx.reserve(65536); // 64k floats = 256KB. Max 1MB VBO.
}

static inline int f2i(float f) { return *(int*)&f; }

void Renderer2D::checkBatch(int shaderId, int texId, int numVerts) {
    if (m_batches.empty()) {
        Batch b;
        b.shaderId = shaderId;
        b.texId = texId;
        b.offset = 0;
        b.count = 0;
        m_batches.push_back(b);
    } else {
        Batch& b = m_batches[m_batches.size() - 1];
        if (b.shaderId != shaderId || b.texId != texId) {
            Batch nb;
            nb.shaderId = shaderId;
            nb.texId = texId;
            nb.offset = b.offset + b.count;
            nb.count = 0;
            m_batches.push_back(nb);
        }
    }
    m_batches[m_batches.size() - 1].count += numVerts;
}

void Renderer2D::end() {
    flush();
}

void Renderer2D::flush() {
    if (m_ui_vtx.empty()) return;
    int floatCount = (int)m_ui_vtx.size();

    // Upload all vertex data
    // Use BufferData to handle resizing automatically and orphan previous buffer
    cmd_push(CMD_BIND_BUFFER, GL_ARRAY_BUFFER, m_vbo, 0, 0, 0, 0, 0);
    cmd_push(CMD_BUFFER_DATA_F32, GL_ARRAY_BUFFER, shine::wasm::ptr_i32(m_ui_vtx.data()), floatCount, GL_DYNAMIC_DRAW, 0, 0, 0);

    // Bind VAO for drawing
    cmd_push(CMD_BIND_VAO, m_vao, 0, 0, 0, 0, 0, 0);

    // Draw batches
    for (unsigned int i = 0; i < m_batches.size(); ++i) {
        const Batch& b = m_batches[i];
        if (b.count == 0) continue;

        if (b.shaderId == 0) { // Tex
            cmd_push(CMD_USE_PROGRAM, m_prog_tex, 0, 0, 0, 0, 0, 0);
            cmd_push(CMD_UNIFORM2F, m_uViewSize, f2i((float)m_viewW), f2i((float)m_viewH), 0,0,0,0);
            cmd_push(CMD_ACTIVE_TEXTURE, 0, 0, 0, 0, 0, 0, 0);
            cmd_push(CMD_BIND_TEXTURE, GL_TEXTURE_2D, b.texId, 0, 0, 0, 0, 0);
            cmd_push(CMD_UNIFORM1I, m_uTex, 0, 0, 0, 0, 0, 0);
            cmd_push(CMD_DRAW_ARRAYS, GL_TRIANGLES, b.offset, b.count, 0, 0, 0, 0);
        } else { // RR
            cmd_push(CMD_USE_PROGRAM, m_prog_rr, 0, 0, 0, 0, 0, 0);
            // Texture? Uniforms already set immediately in drawRoundRect
            // We just need to bind texture if used.
            if (b.texId) {
                cmd_push(CMD_ACTIVE_TEXTURE, 0, 0, 0, 0, 0, 0, 0);
                cmd_push(CMD_BIND_TEXTURE, GL_TEXTURE_2D, b.texId, 0, 0, 0, 0, 0);
                cmd_push(CMD_UNIFORM1I, m_uRR_Tex, 0, 0, 0, 0, 0, 0);
            }
            cmd_push(CMD_DRAW_ARRAYS, GL_TRIANGLES, b.offset, b.count, 0, 0, 0, 0);
        }
    }
    
    // Clear after flush
    // m_ui_vtx.clear(); // DO NOT CLEAR VTX! We need to preserve it for the whole frame because JS executes commands at the end.
    m_batches.clear();
}


float* Renderer2D::allocVtx(int floatCount) {
    const unsigned int oldCount = m_ui_vtx.size();
    const unsigned int need = oldCount + (unsigned int)floatCount;
    if (need > m_ui_vtx.capacity()) {
        // Crash or log error? We can't resize safely if commands are pending.
        // For now, let's just resize and hope we haven't submitted pointers yet?
        // NO, we HAVE submitted pointers (if batching).
        // But we only submit pointers in flush().
        // Wait, if we use flush(), then we submit ALL pointers at end of frame.
        // And we use m_ui_vtx.data() as base + offset.
        // So resizing is SAFE if we only use relative offsets in batches,
        // AND we only take the pointer in flush().
        
        // Let's check flush implementation:
        // cmd_push(..., m_ui_vtx.data(), ...)
        // It uses .data() AT FLUSH TIME.
        // So resizing is SAFE during frame accumulation!
        
        // Wait, why did I think it was unsafe?
        // Ah, because previous code was `drawRectUV` calls `allocVtx` then `cmd_push`.
        // That was Immediate Mode.
        // In Batch Mode, we don't push commands until flush.
        
        // BUT `drawRoundRect` IS STILL IMMEDIATE (partially).
        // It pushes UNIFORM commands immediately.
        // But it pushes DRAW command... wait, I removed the draw command from drawRoundRect.
        // Now drawRoundRect calls `checkBatch` and writes verts.
        // It does NOT call draw.
        // So `flush` handles the draw.
        
        // BUT `drawRoundRect` sets UNIFORMS immediately.
        // And `flush` sets Program and Draws.
        // The Uniforms will apply to the program currently bound?
        // NO. `cmd_push` records the command.
        // The command stream preserves order.
        // 1. Uniform Set (for rect A)
        // 2. Uniform Set (for rect B)
        // ...
        // N. Flush -> Draw Batch (A+B)
        // This is WRONG. Uniforms for A are overwritten by B before Draw is called.
        
        // So we CANNOT batch RoundRects if they differ in uniforms.
        // `checkBatch` must break batch if uniforms change.
        // But `checkBatch` only checks shaderId and texId.
        // We need to check EVERYTHING or just not batch RoundRects.
        
        // Let's modify `checkBatch` to force break for RoundRects?
        // Or better: `drawRoundRect` calls `flush()` immediately after writing its data?
        // Yes. `drawRoundRect` is essentially unbatchable (unless same style).
        // So:
        // 1. Write verts.
        // 2. `checkBatch` (will likely start new batch).
        // 3. `flush()` (submit this batch immediately).
        
        // If we do this, `flush` will submit pending batches.
        // Including the one we just added.
        // So `drawRoundRect` logic:
        // allocVtx -> write -> checkBatch -> flush.
        
        unsigned int newCap = m_ui_vtx.capacity() ? m_ui_vtx.capacity() : 256u;
        while (newCap < need) newCap *= 2u;
        m_ui_vtx.reserve(newCap);
    }
    m_ui_vtx.resize_uninitialized(need);
    return m_ui_vtx.data() + oldCount;
}

void Renderer2D::drawRectColor(float cx, float cy, float w, float h, float r, float g, float b) {
    // Re-use drawRectUV logic but with colored shader?
    // Actually the old code had separate logic for rect color?
    // Looking at old gl_ctx.cpp... 
    // It seemed to reuse the same batch logic or just draw immediately.
    // For now, let's implement a simple version using the same texture shader but with white texture (if possible) or just vertex colors.
    // kVS_TEX uses aCol as UV. It ignores color.
    // We need a shader for colored rects without texture.
    // OR we can use the kVS_RR shader (Rounded Rect) with radius=0 and no texture?
    // Yes, kVS_RR supports uColor.
    
    drawRoundRect(cx, cy, w, h, 0.0f, r, g, b, 1.0f, 0, 0,0,0,0, 0,0,0,0,0, 0,0, 0,0, 0,0,0,0);
}

// static inline int f2i(float f) { return *(int*)&f; }

void Renderer2D::drawRectUV(int texId, float cx, float cy, float w, float h) {
    float* d = allocVtx(6 * 5);
    // int offset = getVtxOffset(d); // No longer needed
    
    checkBatch(0, texId, 6);

    float x1 = cx - w * 0.5f;
    float y1 = cy - h * 0.5f;
    float x2 = cx + w * 0.5f;
    float y2 = cy + h * 0.5f;

    // Quad: (x1,y1,0,0), (x2,y1,1,0), (x1,y2,0,1), (x1,y2,0,1), (x2,y1,1,0), (x2,y2,1,1)
    // 0
    d[0] = x1; d[1] = y1; d[2] = 0.0f; d[3] = 0.0f; d[4] = 0.0f;
    // 1
    d[5] = x2; d[6] = y1; d[7] = 1.0f; d[8] = 0.0f; d[9] = 0.0f;
    // 2
    d[10]= x1; d[11]= y2; d[12]= 0.0f; d[13]= 1.0f; d[14]= 0.0f;
    // 3
    d[15]= x1; d[16]= y2; d[17]= 0.0f; d[18]= 1.0f; d[19]= 0.0f;
    // 4
    d[20]= x2; d[21]= y1; d[22]= 1.0f; d[23]= 0.0f; d[24]= 0.0f;
    // 5
    d[25]= x2; d[26]= y2; d[27]= 1.0f; d[28]= 1.0f; d[29]= 0.0f;
}

void Renderer2D::drawRoundRect(float cx, float cy, float w, float h,
                       float radius_px,
                       float fill_r, float fill_g, float fill_b, float fill_a,
                       int texId,
                       float texTint_r, float texTint_g, float texTint_b, float texTint_a,
                       float border_px,
                       float border_r, float border_g, float border_b, float border_a,
                       float shadow_off_px_x, float shadow_off_px_y,
                       float shadow_blur_px, float shadow_spread_px,
                       float shadow_r, float shadow_g, float shadow_b, float shadow_a) {
    
    // Force flush previous batches if any, because we are about to change uniforms
    // But we haven't implemented partial flush yet.
    // flush() submits EVERYTHING in m_ui_vtx.
    // If we have pending UV rects, they are in m_ui_vtx.
    // We flush them now.
    if (!m_batches.empty()) flush();
    
    // Clear batches and vtx buffer?
    // flush() does NOT clear them in current impl (it just draws).
    // But we need to clear them to start fresh, otherwise we re-draw them!
    // So flush() MUST clear.
    // Let's modify flush() to clear at end.
    
    float* d = allocVtx(6 * 5);
    
    checkBatch(1, texId, 6);

    float x1 = cx - w * 0.5f;
    float y1 = cy - h * 0.5f;
    float x2 = cx + w * 0.5f;
    float y2 = cy + h * 0.5f;

    // UVs are 0..1
    d[0] = x1; d[1] = y1; d[2] = 0.0f; d[3] = 0.0f; d[4] = 0.0f;
    d[5] = x2; d[6] = y1; d[7] = 1.0f; d[8] = 0.0f; d[9] = 0.0f;
    d[10]= x1; d[11]= y2; d[12]= 0.0f; d[13]= 1.0f; d[14]= 0.0f;
    d[15]= x1; d[16]= y2; d[17]= 0.0f; d[18]= 1.0f; d[19]= 0.0f;
    d[20]= x2; d[21]= y1; d[22]= 1.0f; d[23]= 0.0f; d[24]= 0.0f;
    d[25]= x2; d[26]= y2; d[27]= 1.0f; d[28]= 1.0f; d[29]= 0.0f;

    // Ensure Program is active before setting uniforms
    cmd_push(CMD_USE_PROGRAM, m_prog_rr, 0, 0, 0, 0, 0, 0);
    
    // Set Viewport Uniform
    cmd_push(CMD_UNIFORM2F, m_uRR_ViewSize, f2i((float)m_viewW), f2i((float)m_viewH), 0,0,0,0);

    // Uniforms
    // ... (Same uniform setting code)
    float rad_x = (w > 0.1f) ? radius_px / w : 0.0f;
    float rad_y = (h > 0.1f) ? radius_px / h : 0.0f;
    cmd_push(CMD_UNIFORM2F, m_uRR_Rad, f2i(rad_x), f2i(rad_y), 0,0,0,0);
    
    cmd_push(CMD_UNIFORM1I, m_uRR_UseTex, (texId ? 1 : 0), 0,0,0,0,0);
    if (texId) {
        // Texture binding is handled in flush() via checkBatch, but we need to ensure it uses m_uRR_Tex?
        // flush() uses m_uTex for shaderId 0, and what for shaderId 1?
        // In flush(), for RR shader, we didn't bind texture!
        // We should add texture binding to flush() for RR shader too.
        // Or we do it here? No, flush() handles draw call state.
        // We must rely on flush() to bind texture.
    }
    
    cmd_push(CMD_UNIFORM4F, m_uRR_Color, f2i(fill_r), f2i(fill_g), f2i(fill_b), f2i(fill_a), 0, 0);
    cmd_push(CMD_UNIFORM4F, m_uRR_TexTint, f2i(texTint_r), f2i(texTint_g), f2i(texTint_b), f2i(texTint_a), 0, 0);
    cmd_push(CMD_UNIFORM4F, m_uRR_BorderColor, f2i(border_r), f2i(border_g), f2i(border_b), f2i(border_a), 0, 0);
    
    float b_uv = (w+h > 0.1f) ? border_px / ((w+h)*0.5f) : 0.0f;
    cmd_push(CMD_UNIFORM1F, m_uRR_Border, f2i(b_uv), 0,0,0,0,0);
    
    cmd_push(CMD_UNIFORM4F, m_uRR_ShadowColor, f2i(shadow_r), f2i(shadow_g), f2i(shadow_b), f2i(shadow_a), 0, 0);
    
    float off_x = (w > 0.1f) ? shadow_off_px_x / w : 0.0f;
    float off_y = (h > 0.1f) ? shadow_off_px_y / h : 0.0f;
    cmd_push(CMD_UNIFORM2F, m_uRR_ShadowOff, f2i(off_x), f2i(off_y), 0,0,0,0);
    
    float blur_uv = (w+h > 0.1f) ? shadow_blur_px / ((w+h)*0.5f) : 0.0f;
    cmd_push(CMD_UNIFORM1F, m_uRR_ShadowBlur, f2i(blur_uv), 0,0,0,0,0);

    float spread_uv = (w+h > 0.1f) ? shadow_spread_px / ((w+h)*0.5f) : 0.0f;
    cmd_push(CMD_UNIFORM1F, m_uRR_ShadowSpread, f2i(spread_uv), 0,0,0,0,0);

    // Flush immediately to ensure uniforms apply to this draw
    flush();
}

} // namespace graphics
} // namespace shine

extern "C" {

void ui_draw_rect_col(int ctxId, float cx, float cy, float w, float h, float r, float g, float b) {
    shine::graphics::Renderer2D::instance().drawRectColor(cx, cy, w, h, r, g, b);
}

void ui_draw_rect_uv(int ctxId, float cx, float cy, float w, float h, int texId) {
    shine::graphics::Renderer2D::instance().drawRectUV(texId, cx, cy, w, h);
}



} // extern "C"
