#include "renderer_2d.h"
#include "gl_api.h"
#include "../util/wasm_compat.h"
#include "wasm_command_buffer.h"

namespace shine::graphics {

    using namespace shine::wasm;

// Shader strings (unchanged)
static SHINE_CONSTINIT const char kVS_TEX[] =
  "#version 300 es\n"
  "precision mediump float;in vec2 aPos;in vec3 aCol;out vec2 vUV;uniform vec2 uViewSize;void main(){"
  "vUV=aCol.xy;vec2 nPos=(aPos/uViewSize)*2.0-1.0;gl_Position=vec4(nPos.x,-nPos.y,0.0,1.0);}";

static SHINE_CONSTINIT const char kFS_TEX[] =
  "#version 300 es\n"
  "precision mediump float;in vec2 vUV;uniform sampler2D uTex;out vec4 outColor;void main(){outColor=texture(uTex,vUV);}";

static SHINE_CONSTINIT const char kVS_COL[] =
  "#version 300 es\n"
  "precision mediump float;in vec2 aPos;in vec3 aCol;out vec3 vCol;uniform vec2 uViewSize;void main(){"
  "vCol=aCol;vec2 nPos=(aPos/uViewSize)*2.0-1.0;gl_Position=vec4(nPos.x,-nPos.y,0.0,1.0);}";

static SHINE_CONSTINIT const char kFS_COL[] =
  "#version 300 es\n"
  "precision mediump float;in vec3 vCol;out vec4 outColor;void main(){outColor=vec4(vCol,1.0);}";

static SHINE_CONSTINIT const char kU_TEX[] = "uTex";
static SHINE_CONSTINIT const char kU_VIEW[] = "uViewSize";

static SHINE_CONSTINIT const char kFS_RR[] =
  "#version 300 es\n"
  "precision mediump float;in vec2 vUV;uniform vec4 uColor;uniform vec4 uTexTint;uniform vec4 uBorderColor;"
  "uniform float uBorder;uniform vec4 uShadowColor;uniform vec2 uShadowOff;uniform float uShadowBlur;"
  "uniform float uShadowSpread;uniform vec2 uRad;uniform int uUseTex;uniform sampler2D uTex;out vec4 outColor;"
  "float sdfRoundRect(vec2 uv,vec2 rad){vec2 p=uv-vec2(0.5);vec2 q=abs(p)-(vec2(0.5)-rad);"
  "return length(max(q,0.0))+min(max(q.x,q.y),0.0)-min(rad.x,rad.y);}"
  "void main(){vec2 rad=clamp(uRad,vec2(0.0),vec2(0.5));float d=sdfRoundRect(vUV,rad);"
  "float aa=max(fwidth(d),0.0039);float fill=1.0-smoothstep(0.0,aa,d);float t=max(0.0,uBorder);"
  "float inner=1.0-smoothstep(-t,-t+aa,d);float border=clamp(fill-inner,0.0,1.0);vec4 base=uColor;"
  "if(uUseTex!=0)base*=texture(uTex,vUV)*uTexTint;vec4 cFill=vec4(base.rgb,base.a*fill);"
  "vec4 cBorder=vec4(uBorderColor.rgb,uBorderColor.a*border);float ds=sdfRoundRect(vUV-uShadowOff,rad)-uShadowSpread;"
  "float shadow=1.0-smoothstep(0.0,max(0.0,uShadowBlur)+aa,ds);vec4 cShadow=vec4(uShadowColor.rgb,uShadowColor.a*shadow);"
  "vec4 outc=cShadow;outc=outc+cBorder*(1.0-outc.a);outc=outc+cFill*(1.0-outc.a);outColor=outc;}";

static SHINE_CONSTINIT const char kU_RR_USETEX[] = "uUseTex";
static SHINE_CONSTINIT const char kU_RR_COLOR[] = "uColor";
static SHINE_CONSTINIT const char kU_RR_RAD[] = "uRad";
static SHINE_CONSTINIT const char kU_RR_TEXTINT[] = "uTexTint";
static SHINE_CONSTINIT const char kU_RR_BORDERCOLOR[] = "uBorderColor";
static SHINE_CONSTINIT const char kU_RR_BORDER[] = "uBorder";
static SHINE_CONSTINIT const char kU_RR_SHADOWCOLOR[] = "uShadowColor";
static SHINE_CONSTINIT const char kU_RR_SHADOWOFF[] = "uShadowOff";
static SHINE_CONSTINIT const char kU_RR_SHADOWBLUR[] = "uShadowBlur";
static SHINE_CONSTINIT const char kU_RR_SHADOWSPREAD[] = "uShadowSpread";


namespace {
    bool batch_less(const shine::graphics::Renderer2D::Batch& a, const shine::graphics::Renderer2D::Batch& b) {
        if (a.sortKey != b.sortKey) return a.sortKey < b.sortKey;
        if (a.shaderId != b.shaderId) return a.shaderId < b.shaderId;
        return a.texId < b.texId;
    }

    void sort_batches(shine::graphics::Renderer2D::Batch* batches, unsigned int count) {
        for (unsigned int i = 1; i < count; ++i) {
            shine::graphics::Renderer2D::Batch key = batches[i];
            int j = (int)i - 1;
            while (j >= 0 && batch_less(key, batches[j])) {
                batches[j + 1] = batches[j];
                --j;
            }
            batches[j + 1] = key;
        }
    }
} // anonymous namespace

// Safe static instance
static Renderer2D s_renderer;

Renderer2D& Renderer2D::instance() {
    return s_renderer;
}

#define offsetof(t, d) __builtin_offsetof(t, d)

void Renderer2D::init(int ctx) {
    m_ctx = ctx;
    // VBO & VAO
    m_vbo = gl_create_buffer(ctx);
    m_vao = gl_create_vertex_array(ctx);
    
    gl_bind_vertex_array(ctx, m_vao);
    gl_bind_buffer(ctx, GL_ARRAY_BUFFER, m_vbo);
    gl_enable_attribs(ctx);
    gl_bind_vertex_array(ctx, 0);

    m_ui_vtx.reserve(65536);

    // Programs
    m_prog_tex = gl_create_program_from_source(ctx, kVS_TEX, kFS_TEX);
    m_prog_col = gl_create_program_from_source(ctx, kVS_COL, kFS_COL);
    m_prog_rr = gl_create_program_from_source(ctx, kVS_TEX, kFS_RR);

    // Uniforms (Static const table in .rodata)
    struct UDef { unsigned short offset; unsigned char progIdx; const char* name; };
    
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
    static SHINE_CONSTINIT const UDef kUniforms[] = {
        { (unsigned short)offsetof(Renderer2D, m_uTex), 0, kU_TEX },
        { (unsigned short)offsetof(Renderer2D, m_uViewSize), 0, kU_VIEW },
        { (unsigned short)offsetof(Renderer2D, m_uCol_ViewSize), 1, kU_VIEW },
        { (unsigned short)offsetof(Renderer2D, m_uRR_ViewSize), 2, kU_VIEW },
        { (unsigned short)offsetof(Renderer2D, m_uRR_Tex), 2, kU_TEX }
    };
    
    static SHINE_CONSTINIT const char* const kRRUniformNames[] = {
        kU_RR_RAD, kU_RR_USETEX, kU_RR_COLOR, kU_RR_TEXTINT,
        kU_RR_BORDERCOLOR, kU_RR_BORDER, kU_RR_SHADOWCOLOR,
        kU_RR_SHADOWOFF, kU_RR_SHADOWBLUR, kU_RR_SHADOWSPREAD
    };
#pragma clang diagnostic pop
    
    const int progs[] = { m_prog_tex, m_prog_col, m_prog_rr };
    
    #pragma clang loop unroll(disable)
    for (const auto& u : kUniforms) {
        int* target = (int*)((char*)this + u.offset);
        *target = gl_get_uniform_location(ctx, progs[u.progIdx], ptr_i32(u.name), raw_strlen(u.name));
    }

    #pragma clang loop unroll(disable)
    for (int i = 0; i < 10; ++i) {
        const char* name = kRRUniformNames[i];
        m_rrUniformLocs[i] = gl_get_uniform_location(ctx, m_prog_rr, ptr_i32(name), raw_strlen(name));
    }

    // Initial Buffer
    gl_bind_buffer(ctx, GL_ARRAY_BUFFER, m_vbo);
    gl_buffer_data_f32(ctx, GL_ARRAY_BUFFER, 0, 256 * 1024, GL_DYNAMIC_DRAW); 
}

void Renderer2D::begin() {
    m_ui_vtx.clear();
    m_batches.clear();
    m_rr_blocks.clear();
    m_rr_states.clear();
    m_need_sort = false;
    if (m_ui_vtx.capacity() < 65536) {
        m_ui_vtx.reserve(65536);
    }
    if (m_batches.capacity() < 256) {
        m_batches.reserve(256);
    }
}

// ---- 标记为 noinline 以减小代码尺寸 ----
__attribute__((noinline))
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

__attribute__((noinline))
void Renderer2D::checkBatch(int shaderId, int texId, int firstVertex, int numVerts, unsigned int sortKey) {
    if (!m_batches.empty()) {
        Batch& last = m_batches.back();
        if (last.shaderId == shaderId && last.texId == texId && last.sortKey == sortKey) {
            last.count += numVerts;
            return;
        }
    }
    
    Batch b;
    b.shaderId = shaderId;
    b.texId = texId;
    b.offset = firstVertex;
    b.count = numVerts;
    b.sortKey = sortKey;
    m_batches.push_back(b);
    if (sortKey != 0) m_need_sort = true;
}

__attribute__((noinline))
void Renderer2D::checkBatchRR(int texId, const RRUniformState& rr, int firstVertex, int numVerts, unsigned int sortKey) {
    if (!m_batches.empty()) {
        Batch& last = m_batches.back();
        if (last.shaderId == 1 && last.texId == texId && last.sortKey == sortKey &&
            last.rrIndex >= 0 && raw_memcmp(&m_rr_states[last.rrIndex], &rr, sizeof(RRUniformState)) == 0) {
            last.count += numVerts;
            return;
        }
    }

    Batch b;
    b.shaderId = 1;
    b.texId = texId;
    b.offset = firstVertex;
    b.count = numVerts;
    b.sortKey = sortKey;
    b.rrIndex = m_rr_states.size();
    m_rr_states.push_back(rr);
    m_batches.push_back(b);
    if (sortKey != 0) m_need_sort = true;
}

void Renderer2D::end() {
    flush();
}

inline void Renderer2D::switchShader(const Batch& b, int& curShaderId, unsigned int& setupMask, CommandBuffer::Pass& pass) {
    curShaderId = b.shaderId;
    int prog = (curShaderId == 0) ? m_prog_tex : ((curShaderId == 2) ? m_prog_col : m_prog_rr);
    pass.push2(CMD_USE_PROGRAM, prog);

    const int viewW_i = f2i((float)m_viewW);
    const int viewH_i = f2i((float)m_viewH);

    if (curShaderId == 0) { // Tex
        if (!(setupMask & 1)) {
            pass.push5(CMD_SETUP_VIEW_SAMPLER2D, m_uViewSize, viewW_i, viewH_i, m_uTex);
            setupMask |= 1;
        }
    } else if (curShaderId == 2) { // Col
        if (!(setupMask & 4)) {
            pass.push4(CMD_UNIFORM2F, m_uCol_ViewSize, viewW_i, viewH_i);
            setupMask |= 4;
        }
    } else { // RR
        if (!(setupMask & 2)) {
            pass.push4(CMD_UNIFORM2F, m_uRR_ViewSize, viewW_i, viewH_i);
            pass.push2(CMD_UNIFORM1I, m_uRR_Tex);
            setupMask |= 2;
        }
    }
}

void Renderer2D::flush() {
   if (m_ui_vtx.empty()) return;
    
    // 清理空批次
    unsigned int write = 0;
    const unsigned int count = m_batches.size();
    for (unsigned int i = 0; i < count; ++i) {
        if (m_batches[i].count != 0) {
            m_batches[write++] = m_batches[i];
        }
    }
    m_batches.resize(write);
    if (write == 0) return;

    // 排序批次
    if (m_need_sort && write > 1u) {
        sort_batches(m_batches.data(), write);
    }


    CommandBuffer::Pass pass = g_cmd_buffer.begin_pass();

    // 上传顶点数据
    pass.push3(CMD_BIND_BUFFER, GL_ARRAY_BUFFER, m_vbo);
    const int uploadCount = (int)m_ui_vtx.size();
    if (m_lastUploadCount > 0 && uploadCount <= m_lastUploadCount) {
        pass.push5(CMD_BUFFER_SUB_DATA_F32, GL_ARRAY_BUFFER, 0, ptr_i32(m_ui_vtx.data()), uploadCount);
    } else {
        pass.push(CMD_BUFFER_DATA_F32, GL_ARRAY_BUFFER, ptr_i32(m_ui_vtx.data()), uploadCount, GL_DYNAMIC_DRAW, 0, 0, 0);
        m_lastUploadCount = uploadCount;
    }
    pass.push2(CMD_BIND_VAO, m_vao);

    // 渲染状态
    int curShaderId = -1;
    int curTexId = -1;
    unsigned int setupMask = 0;
    RRUniformState lastRR;
    bool hasLastRR = false;
    
    const unsigned int batch_count = m_batches.size();
    const Batch* batch_data = m_batches.data();
    for (unsigned int i = 0; i < batch_count; ++i) {
        const Batch& b = batch_data[i];

        // 切换着色器
        if (b.shaderId != curShaderId) {
            switchShader(b, curShaderId, setupMask, pass);
        }

        // 更新 RR 专属 uniform
        if (curShaderId == 1 && b.rrIndex >= 0) {
            updateRRUniforms(m_rr_states[b.rrIndex], lastRR, hasLastRR, pass);
        }

        // 绑定纹理
        if (b.texId != curTexId) {
            curTexId = b.texId;
            if (curShaderId != 2 && b.texId != 0) {
                pass.push3(CMD_BIND_TEXTURE, GL_TEXTURE_2D, b.texId);
            }
        }

        // 绘制
        pass.push4(CMD_DRAW_ARRAYS, GL_TRIANGLES, b.offset, b.count);
    }
    
    m_batches.clear();
}


void Renderer2D::updateRRUniforms(const RRUniformState& b, RRUniformState& last, bool& hasLastRR, CommandBuffer::Pass& pass) {
       if (hasLastRR && raw_memcmp(&b, &last, sizeof(RRUniformState)) == 0)
        return;

    const unsigned int base = m_rr_blocks.size();
    m_rr_blocks.resize_uninitialized(base + 34u);
    int* out = m_rr_blocks.data() + base;

    // 批量拷贝 uniform 位置（10 个 int）
    raw_memcpy(out, m_rrUniformLocs, sizeof(m_rrUniformLocs));

    // 批量拷贝 uniform 值（RRUniformState 共 24 个 int）
    raw_memcpy(out + 10, &b, sizeof(RRUniformState));

    pass.push2(CMD_UNIFORM_RR_BLOCK, ptr_i32(out));

    raw_memcpy(&last, &b, sizeof(RRUniformState));
    hasLastRR = true;
}

// ---- 简化的绘图函数 ----
static inline void writeQuadVtxCol(float* d, float cx, float cy, float w, float h, float r, float g, float b) {
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

static inline void writeQuadVtxUV(float* d, float cx, float cy, float w, float h) {
    float x1 = cx - w * 0.5f;
    float y1 = cy - h * 0.5f;
    float x2 = cx + w * 0.5f;
    float y2 = cy + h * 0.5f;
    d[0] = x1; d[1] = y1; d[2] = 0.0f; d[3] = 0.0f; d[4] = 0.0f;
    d[5] = x2; d[6] = y1; d[7] = 1.0f; d[8] = 0.0f; d[9] = 0.0f;
    d[10]= x1; d[11]= y2; d[12]= 0.0f; d[13]= 1.0f; d[14]= 0.0f;
    d[15]= x1; d[16]= y2; d[17]= 0.0f; d[18]= 1.0f; d[19]= 0.0f;
    d[20]= x2; d[21]= y1; d[22]= 1.0f; d[23]= 0.0f; d[24]= 0.0f;
    d[25]= x2; d[26]= y2; d[27]= 1.0f; d[28]= 1.0f; d[29]= 0.0f;
}

void Renderer2D::drawRectColor(float cx, float cy, float w, float h, float r, float g, float b) {
    int firstVertex = 0;
    float* d = allocVtx(6 * 5, &firstVertex);
    checkBatch(2, 0, firstVertex, 6, 0);
    writeQuadVtxCol(d, cx, cy, w, h, r, g, b);
}

void Renderer2D::drawRectUV(int texId, float cx, float cy, float w, float h) {
    int firstVertex = 0;
    float* d = allocVtx(6 * 5, &firstVertex);
    checkBatch(0, texId, firstVertex, 6, 0);
    writeQuadVtxUV(d, cx, cy, w, h);
}

void Renderer2D::drawRectColorSorted(float cx, float cy, float w, float h, float r, float g, float b, unsigned int sortKey) {
    int firstVertex = 0;
    float* d = allocVtx(6 * 5, &firstVertex);
    checkBatch(2, 0, firstVertex, 6, sortKey);
    writeQuadVtxCol(d, cx, cy, w, h, r, g, b);
}

void Renderer2D::drawRectUVSorted(int texId, float cx, float cy, float w, float h, unsigned int sortKey) {
    int firstVertex = 0;
    float* d = allocVtx(6 * 5, &firstVertex);
    checkBatch(0, texId, firstVertex, 6, sortKey);
    writeQuadVtxUV(d, cx, cy, w, h);
}

// ---- 优化后的 drawRoundRect ----
void Renderer2D::drawRoundRect(const Rect& rect, const RoundRectStyle& style) {
    int firstVertex = 0;
    float* d = allocVtx(6 * 5, &firstVertex);

    // 假设 w 和 h > 0 (有效渲染尺寸)
    const float w = rect.w;
    const float h = rect.h;
    const float inv_w = 1.0f / w;
    const float inv_h = 1.0f / h;
    const float inv_sum = 2.0f / (w + h);

    RRUniformState rr;
    rr.useTex = (style.texId != 0) ? 1 : 0;
    rr.radX = f2i(style.radius_px * inv_w);
    rr.radY = f2i(style.radius_px * inv_h);
    rr.colorR = f2i(style.fill.r);
    rr.colorG = f2i(style.fill.g);
    rr.colorB = f2i(style.fill.b);
    rr.colorA = f2i(style.fill.a);
    rr.texTintR = f2i(style.texTint.r);
    rr.texTintG = f2i(style.texTint.g);
    rr.texTintB = f2i(style.texTint.b);
    rr.texTintA = f2i(style.texTint.a);
    rr.borderColorR = f2i(style.borderColor.r);
    rr.borderColorG = f2i(style.borderColor.g);
    rr.borderColorB = f2i(style.borderColor.b);
    rr.borderColorA = f2i(style.borderColor.a);
    rr.border = f2i(style.border_px * inv_sum);
    rr.shadowColorR = f2i(style.shadowColor.r);
    rr.shadowColorG = f2i(style.shadowColor.g);
    rr.shadowColorB = f2i(style.shadowColor.b);
    rr.shadowColorA = f2i(style.shadowColor.a);
    rr.shadowOffX = f2i(style.shadow_off_x * inv_w);
    rr.shadowOffY = f2i(style.shadow_off_y * inv_h);
    rr.shadowBlur = f2i(style.shadow_blur * inv_sum);
    rr.shadowSpread = f2i(style.shadow_spread * inv_sum);

    checkBatchRR(style.texId, rr, firstVertex, 6, 0);

    const float cx = rect.cx;
    const float cy = rect.cy;
    float x1 = cx - w * 0.5f;
    float y1 = cy - h * 0.5f;
    float x2 = cx + w * 0.5f;
    float y2 = cy + h * 0.5f;

    // 顶点赋值 (布局: x, y, u, v, _pad)
    writeQuadVtxUV(d, cx, cy, w, h);
}

} // namespace shine::graphics


extern "C" {

void ui_draw_rect_col(int ctxId, float cx, float cy, float w, float h, float r, float g, float b) {
    shine::graphics::Renderer2D::instance().drawRectColor(cx, cy, w, h, r, g, b);
}

void ui_draw_rect_uv(int ctxId, float cx, float cy, float w, float h, int texId) {
    shine::graphics::Renderer2D::instance().drawRectUV(texId, cx, cy, w, h);
}

} // extern "C"