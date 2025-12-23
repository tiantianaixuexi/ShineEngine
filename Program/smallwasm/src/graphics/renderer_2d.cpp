#include "renderer_2d.h"
#include "gl_api.h"
#include "../util/wasm_compat.h"
#include "wasm_command_buffer.h"

namespace shine::graphics {

	using namespace shine::wasm;

// Shader strings
static const char kVS_TEX[] =
  "#version 300 es\n"
  "precision mediump float;\n"
  "in vec2 aPos;\n"
  "in vec3 aCol;\n"
  "out vec2 vUV;\n"
  "uniform vec2 uViewSize;\n"
  "void main(){\n"
  "  vUV = aCol.xy;\n"
  "  vec2 nPos = (aPos / uViewSize) * 2.0 - 1.0;\n"
  "  gl_Position = vec4(nPos.x, -nPos.y, 0.0, 1.0);\n" // Y flip for top-left 0,0
  "}\n";

static const char kFS_TEX[] =
  "#version 300 es\n"
  "precision mediump float;\n"
  "in vec2 vUV;\n"
  "uniform sampler2D uTex;\n"
  "out vec4 outColor;\n"
  "void main(){ outColor = texture(uTex, vUV); }\n";

static const char kVS_COL[] =
  "#version 300 es\n"
  "precision mediump float;\n"
  "in vec2 aPos;\n"
  "in vec3 aCol;\n"
  "out vec3 vCol;\n"
  "uniform vec2 uViewSize;\n"
  "void main(){\n"
  "  vCol = aCol;\n"
  "  vec2 nPos = (aPos / uViewSize) * 2.0 - 1.0;\n"
  "  gl_Position = vec4(nPos.x, -nPos.y, 0.0, 1.0);\n"
  "}\n";

static const char kFS_COL[] =
  "#version 300 es\n"
  "precision mediump float;\n"
  "in vec3 vCol;\n"
  "out vec4 outColor;\n"
  "void main(){ outColor = vec4(vCol, 1.0); }\n";

static const char kU_TEX[] = "uTex";
static const char kU_VIEW[] = "uViewSize";

static const char kFS_RR[] =
  "#version 300 es\n"
  "precision mediump float;\n"
  "in vec2 vUV;\n"
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
  "out vec4 outColor;\n"
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
  "  float aa = max(fwidth(d), 1.0/256.0);\n"
  "  float fill = 1.0 - smoothstep(0.0, aa, d);\n"
  "  float t = max(0.0, uBorder);\n"
  "  float inner = 1.0 - smoothstep(-t, -t + aa, d);\n"
  "  float border = clamp(fill - inner, 0.0, 1.0);\n"
  "  vec4 base = uColor;\n"
  "  if(uUseTex!=0){ base *= texture(uTex, vUV) * uTexTint; }\n"
  "  vec4 cFill = vec4(base.rgb, base.a * fill);\n"
  "  vec4 cBorder = vec4(uBorderColor.rgb, uBorderColor.a * border);\n"
  "  float ds = sdfRoundRect(vUV - uShadowOff, rad) - uShadowSpread;\n"
  "  float sb = max(0.0, uShadowBlur);\n"
  "  float shadow = 1.0 - smoothstep(0.0, sb + aa, ds);\n"
  "  vec4 cShadow = vec4(uShadowColor.rgb, uShadowColor.a * shadow);\n"
  "  vec4 outc = cShadow;\n"
  "  outc = outc + cBorder * (1.0 - outc.a);\n"
  "  outc = outc + cFill * (1.0 - outc.a);\n"
  "  outColor = outc;\n"
  "}\n";

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

    int vs_col = gl_create_shader(ctx, GL_VERTEX_SHADER, shine::wasm::ptr_i32(kVS_COL), sizeof(kVS_COL)-1);
    int fs_col = gl_create_shader(ctx, GL_FRAGMENT_SHADER, shine::wasm::ptr_i32(kFS_COL), sizeof(kFS_COL)-1);
    m_prog_col = gl_create_program(ctx, vs_col, fs_col);
    m_uCol_ViewSize = gl_get_uniform_location(ctx, m_prog_col, shine::wasm::ptr_i32(kU_VIEW), sizeof(kU_VIEW)-1);

    // RR shader
    int vs2 = gl_create_shader(ctx, GL_VERTEX_SHADER, shine::wasm::ptr_i32(kVS_TEX), sizeof(kVS_TEX)-1);
    int fs2 = gl_create_shader(ctx, GL_FRAGMENT_SHADER, shine::wasm::ptr_i32(kFS_RR), sizeof(kFS_RR)-1);
    m_prog_rr = gl_create_program(ctx, vs2, fs2);
    m_uRR_ViewSize = gl_get_uniform_location(ctx, m_prog_rr, shine::wasm::ptr_i32(kU_VIEW), sizeof(kU_VIEW)-1);
    m_uRR_Tex = gl_get_uniform_location(ctx, m_prog_rr, shine::wasm::ptr_i32(kU_TEX), sizeof(kU_TEX)-1);
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



void Renderer2D::checkBatch(int shaderId, int texId, int firstVertex, int numVerts) {
    if (m_batches.empty()) {
        Batch b;
        b.shaderId = shaderId;
        b.texId = texId;
        b.offset = firstVertex;
        b.count = 0;
        m_batches.push_back(b);
    } else {
        Batch& b = m_batches[m_batches.size() - 1];
        if (b.shaderId != shaderId || b.texId != texId) {
            Batch nb;
            nb.shaderId = shaderId;
            nb.texId = texId;
            nb.offset = firstVertex;
            nb.count = 0;
            m_batches.push_back(nb);
        }
    }
    m_batches[m_batches.size() - 1].count += numVerts;
}

void Renderer2D::checkBatchRR(int texId, const Renderer2D::RRUniformState& rr, int firstVertex, int numVerts) {
    if (m_batches.empty()) {
        Batch b;
        b.shaderId = 1;
        b.texId = texId;
        b.offset = firstVertex;
        b.count = 0;
        b.rr = rr;
        m_batches.push_back(b);
    } else {
        Batch& b = m_batches[m_batches.size() - 1];
        if (b.shaderId != 1 || b.texId != texId || !(b.rr == rr)) {
            Batch nb;
            nb.shaderId = 1;
            nb.texId = texId;
            nb.offset = firstVertex;
            nb.count = 0;
            nb.rr = rr;
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
    bool rrNeedsTex = false;
    for (unsigned int i = 0; i < m_batches.size(); ++i) {
        const Batch& b = m_batches[i];
        if (b.shaderId == 1 && b.texId != 0) {
            rrNeedsTex = true;
            break;
        }
    }

    // Upload all vertex data
    // Use BufferData to handle resizing automatically and orphan previous buffer
    cmd_push(CMD_BIND_BUFFER, GL_ARRAY_BUFFER, m_vbo, 0, 0, 0, 0, 0);
    cmd_push(CMD_BUFFER_DATA_F32, GL_ARRAY_BUFFER, shine::wasm::ptr_i32(m_ui_vtx.data()), floatCount, GL_DYNAMIC_DRAW, 0, 0, 0);

    // Bind VAO for drawing
    cmd_push(CMD_BIND_VAO, m_vao, 0, 0, 0, 0, 0, 0);

    // Draw batches
    int curShaderId = -1;
    int curTexId = -1;
    bool texSetup = false;
    bool colSetup = false;
    bool rrSetup = false;
    bool rrTexUnitReady = false;
    bool rrSamplerReady = false;
    RRUniformState lastRR;
    bool hasLastRR = false;
    for (unsigned int i = 0; i < m_batches.size(); ++i) {
        const Batch& b = m_batches[i];
        if (b.count == 0) continue;

        if (b.shaderId != curShaderId) {
            curShaderId = b.shaderId;
            if (curShaderId == 0) {
                cmd_push(CMD_USE_PROGRAM, m_prog_tex, 0, 0, 0, 0, 0, 0);
                if (!texSetup) {
                    cmd_push(CMD_SETUP_VIEW_SAMPLER2D, m_uViewSize, f2i((float)m_viewW), f2i((float)m_viewH), m_uTex, 0, 0, 0);
                    texSetup = true;
                }
            } else if (curShaderId == 2) {
                cmd_push(CMD_USE_PROGRAM, m_prog_col, 0, 0, 0, 0, 0, 0);
                if (!colSetup) {
                    cmd_push(CMD_UNIFORM2F, m_uCol_ViewSize, f2i((float)m_viewW), f2i((float)m_viewH), 0, 0, 0, 0);
                    colSetup = true;
                }
            } else {
                cmd_push(CMD_USE_PROGRAM, m_prog_rr, 0, 0, 0, 0, 0, 0);
                if (rrNeedsTex) {
                    if (!rrSetup) {
                        cmd_push(CMD_SETUP_VIEW_SAMPLER2D, m_uRR_ViewSize, f2i((float)m_viewW), f2i((float)m_viewH), m_uRR_Tex, 0, 0, 0);
                        rrSetup = true;
                        rrTexUnitReady = true;
                        rrSamplerReady = true;
                    } else {
                        if (!rrTexUnitReady) {
                            cmd_push(CMD_ACTIVE_TEXTURE, 0, 0, 0, 0, 0, 0, 0);
                            rrTexUnitReady = true;
                        }
                        if (!rrSamplerReady) {
                            cmd_push(CMD_UNIFORM1I, m_uRR_Tex, 0, 0, 0, 0, 0, 0);
                            rrSamplerReady = true;
                        }
                    }
                } else {
                    if (!rrSetup) {
                        cmd_push(CMD_UNIFORM2F, m_uRR_ViewSize, f2i((float)m_viewW), f2i((float)m_viewH), 0, 0, 0, 0);
                        rrSetup = true;
                    }
                }
            }
        }

        if (curShaderId == 1) {
            if (!hasLastRR || b.rr.radX != lastRR.radX || b.rr.radY != lastRR.radY) {
                cmd_push(CMD_UNIFORM2F, m_uRR_Rad, b.rr.radX, b.rr.radY, 0, 0, 0, 0);
            }
            if (!hasLastRR || b.rr.useTex != lastRR.useTex) {
                cmd_push(CMD_UNIFORM1I, m_uRR_UseTex, b.rr.useTex, 0, 0, 0, 0, 0);
            }
            if (!hasLastRR || b.rr.colorR != lastRR.colorR || b.rr.colorG != lastRR.colorG || b.rr.colorB != lastRR.colorB || b.rr.colorA != lastRR.colorA) {
                cmd_push(CMD_UNIFORM4F, m_uRR_Color, b.rr.colorR, b.rr.colorG, b.rr.colorB, b.rr.colorA, 0, 0);
            }
            if (!hasLastRR || b.rr.texTintR != lastRR.texTintR || b.rr.texTintG != lastRR.texTintG || b.rr.texTintB != lastRR.texTintB || b.rr.texTintA != lastRR.texTintA) {
                cmd_push(CMD_UNIFORM4F, m_uRR_TexTint, b.rr.texTintR, b.rr.texTintG, b.rr.texTintB, b.rr.texTintA, 0, 0);
            }
            if (!hasLastRR || b.rr.borderColorR != lastRR.borderColorR || b.rr.borderColorG != lastRR.borderColorG || b.rr.borderColorB != lastRR.borderColorB || b.rr.borderColorA != lastRR.borderColorA) {
                cmd_push(CMD_UNIFORM4F, m_uRR_BorderColor, b.rr.borderColorR, b.rr.borderColorG, b.rr.borderColorB, b.rr.borderColorA, 0, 0);
            }
            if (!hasLastRR || b.rr.border != lastRR.border) {
                cmd_push(CMD_UNIFORM1F, m_uRR_Border, b.rr.border, 0, 0, 0, 0, 0);
            }
            if (!hasLastRR || b.rr.shadowColorR != lastRR.shadowColorR || b.rr.shadowColorG != lastRR.shadowColorG || b.rr.shadowColorB != lastRR.shadowColorB || b.rr.shadowColorA != lastRR.shadowColorA) {
                cmd_push(CMD_UNIFORM4F, m_uRR_ShadowColor, b.rr.shadowColorR, b.rr.shadowColorG, b.rr.shadowColorB, b.rr.shadowColorA, 0, 0);
            }
            if (!hasLastRR || b.rr.shadowOffX != lastRR.shadowOffX || b.rr.shadowOffY != lastRR.shadowOffY) {
                cmd_push(CMD_UNIFORM2F, m_uRR_ShadowOff, b.rr.shadowOffX, b.rr.shadowOffY, 0, 0, 0, 0);
            }
            if (!hasLastRR || b.rr.shadowBlur != lastRR.shadowBlur) {
                cmd_push(CMD_UNIFORM1F, m_uRR_ShadowBlur, b.rr.shadowBlur, 0, 0, 0, 0, 0);
            }
            if (!hasLastRR || b.rr.shadowSpread != lastRR.shadowSpread) {
                cmd_push(CMD_UNIFORM1F, m_uRR_ShadowSpread, b.rr.shadowSpread, 0, 0, 0, 0, 0);
            }
            lastRR = b.rr;
            hasLastRR = true;
        }

        if (b.texId != curTexId) {
            curTexId = b.texId;
            if (curShaderId == 0) {
                cmd_push(CMD_BIND_TEXTURE, GL_TEXTURE_2D, b.texId, 0, 0, 0, 0, 0);
            } else if (curShaderId == 1) {
                if (b.texId) {
                    cmd_push(CMD_BIND_TEXTURE, GL_TEXTURE_2D, b.texId, 0, 0, 0, 0, 0);
                }
            }
        }

        cmd_push(CMD_DRAW_ARRAYS, GL_TRIANGLES, b.offset, b.count, 0, 0, 0, 0);
    }
    
    // Clear after flush
    // m_ui_vtx.clear(); // DO NOT CLEAR VTX! We need to preserve it for the whole frame because JS executes commands at the end.
    m_batches.clear();
}


float* Renderer2D::allocVtx(int floatCount, int* out_first_vertex) {
    const unsigned int oldCount = m_ui_vtx.size();
    if (out_first_vertex) *out_first_vertex = (int)(oldCount / 5u);
    const unsigned int need = oldCount + (unsigned int)floatCount;
    if (need > m_ui_vtx.capacity()) {
        unsigned int newCap = m_ui_vtx.capacity() ? m_ui_vtx.capacity() : 256u;
        while (newCap < need) newCap *= 2u;
        m_ui_vtx.reserve(newCap);
    }
    m_ui_vtx.resize_uninitialized(need);
    return m_ui_vtx.data() + oldCount;
}

void Renderer2D::drawRectColor(float cx, float cy, float w, float h, float r, float g, float b) {
    int firstVertex = 0;
    float* d = allocVtx(6 * 5, &firstVertex);
    checkBatch(2, 0, firstVertex, 6);

    float x1 = cx - w * 0.5f;
    float y1 = cy - h * 0.5f;
    float x2 = cx + w * 0.5f;
    float y2 = cy + h * 0.5f;

    d[0] = x1; d[1] = y1; d[2] = r; d[3] = g; d[4] = b;
    d[5] = x2; d[6] = y1; d[7] = r; d[8] = g; d[9] = b;
    d[10]= x1; d[11]= y2; d[12]= r; d[13]= g; d[14]= b;
    d[15]= x1; d[16]= y2; d[17]= r; d[18]= g; d[19]= b;
    d[20]= x2; d[21]= y1; d[22]= r; d[23]= g; d[24]= b;
    d[25]= x2; d[26]= y2; d[27]= r; d[28]= g; d[29]= b;
}

// static inline int f2i(float f) { return *(int*)&f; }

void Renderer2D::drawRectUV(int texId, float cx, float cy, float w, float h) {
    int firstVertex = 0;
    float* d = allocVtx(6 * 5, &firstVertex);
    // int offset = getVtxOffset(d); // No longer needed
    
    checkBatch(0, texId, firstVertex, 6);

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
    int firstVertex = 0;
    float* d = allocVtx(6 * 5, &firstVertex);

    RRUniformState rr;
    rr.useTex = (texId ? 1 : 0);

    const float rad_x = (w > 0.1f) ? radius_px / w : 0.0f;
    const float rad_y = (h > 0.1f) ? radius_px / h : 0.0f;
    rr.radX = f2i(rad_x);
    rr.radY = f2i(rad_y);

    rr.colorR = f2i(fill_r);
    rr.colorG = f2i(fill_g);
    rr.colorB = f2i(fill_b);
    rr.colorA = f2i(fill_a);

    rr.texTintR = f2i(texTint_r);
    rr.texTintG = f2i(texTint_g);
    rr.texTintB = f2i(texTint_b);
    rr.texTintA = f2i(texTint_a);

    rr.borderColorR = f2i(border_r);
    rr.borderColorG = f2i(border_g);
    rr.borderColorB = f2i(border_b);
    rr.borderColorA = f2i(border_a);

    const float b_uv = (w + h > 0.1f) ? border_px / ((w + h) * 0.5f) : 0.0f;
    rr.border = f2i(b_uv);

    rr.shadowColorR = f2i(shadow_r);
    rr.shadowColorG = f2i(shadow_g);
    rr.shadowColorB = f2i(shadow_b);
    rr.shadowColorA = f2i(shadow_a);

    const float off_x = (w > 0.1f) ? shadow_off_px_x / w : 0.0f;
    const float off_y = (h > 0.1f) ? shadow_off_px_y / h : 0.0f;
    rr.shadowOffX = f2i(off_x);
    rr.shadowOffY = f2i(off_y);

    const float blur_uv = (w + h > 0.1f) ? shadow_blur_px / ((w + h) * 0.5f) : 0.0f;
    rr.shadowBlur = f2i(blur_uv);

    const float spread_uv = (w + h > 0.1f) ? shadow_spread_px / ((w + h) * 0.5f) : 0.0f;
    rr.shadowSpread = f2i(spread_uv);

    checkBatchRR(texId, rr, firstVertex, 6);

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
}

} // namespace shine

extern "C" {

void ui_draw_rect_col(int ctxId, float cx, float cy, float w, float h, float r, float g, float b) {
    shine::graphics::Renderer2D::instance().drawRectColor(cx, cy, w, h, r, g, b);
}

void ui_draw_rect_uv(int ctxId, float cx, float cy, float w, float h, int texId) {
    shine::graphics::Renderer2D::instance().drawRectUV(texId, cx, cy, w, h);
}



} // extern "C"
