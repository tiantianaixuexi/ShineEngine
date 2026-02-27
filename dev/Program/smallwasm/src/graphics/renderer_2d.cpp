#include "renderer_2d.h"
#include "gl_api.h"
#include "../util/wasm_compat.h"
#include "../util/sort_utils.h"
#include "wasm_command_buffer.h"

namespace shine::graphics {

	using namespace shine::wasm;

// Shader strings
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
        { (unsigned short)offsetof(Renderer2D, m_uRR_Tex), 2, kU_TEX },
        { (unsigned short)offsetof(Renderer2D, m_uRR_UseTex), 2, kU_RR_USETEX },
        { (unsigned short)offsetof(Renderer2D, m_uRR_Color), 2, kU_RR_COLOR },
        { (unsigned short)offsetof(Renderer2D, m_uRR_Rad), 2, kU_RR_RAD },
        { (unsigned short)offsetof(Renderer2D, m_uRR_TexTint), 2, kU_RR_TEXTINT },
        { (unsigned short)offsetof(Renderer2D, m_uRR_BorderColor), 2, kU_RR_BORDERCOLOR },
        { (unsigned short)offsetof(Renderer2D, m_uRR_Border), 2, kU_RR_BORDER },
        { (unsigned short)offsetof(Renderer2D, m_uRR_ShadowColor), 2, kU_RR_SHADOWCOLOR },
        { (unsigned short)offsetof(Renderer2D, m_uRR_ShadowOff), 2, kU_RR_SHADOWOFF },
        { (unsigned short)offsetof(Renderer2D, m_uRR_ShadowBlur), 2, kU_RR_SHADOWBLUR },
        { (unsigned short)offsetof(Renderer2D, m_uRR_ShadowSpread), 2, kU_RR_SHADOWSPREAD }
    };
#pragma clang diagnostic pop
    
    const int progs[] = { m_prog_tex, m_prog_col, m_prog_rr };
    
    #pragma clang loop unroll(disable)
    for (const auto& u : kUniforms) {
        int* target = (int*)((char*)this + u.offset);
        int prog = progs[u.progIdx];
        *target = gl_get_uniform_location(ctx, prog, ptr_i32(u.name), raw_strlen(u.name));
    }

    // Initial Buffer
    gl_bind_buffer(ctx, GL_ARRAY_BUFFER, m_vbo);
    gl_buffer_data_f32(ctx, GL_ARRAY_BUFFER, 0, 256 * 1024, GL_DYNAMIC_DRAW); 
}

void Renderer2D::begin() {
    m_ui_vtx.clear();
    m_batches.clear();
    m_rr_blocks.clear();
    m_need_sort = false;
    // Pre-allocate to avoid pointer invalidation and memory movement during frame
    if (m_ui_vtx.capacity() < 65536) {
        m_ui_vtx.reserve(65536); // 64k floats = 256KB. Max 1MB VBO.
    }
    if (m_batches.capacity() < 256) {
        m_batches.reserve(256);
    }
}



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

void Renderer2D::checkBatchRR(int texId, const Renderer2D::RRUniformState& rr, int firstVertex, int numVerts, unsigned int sortKey) {
    if (!m_batches.empty()) {
        Batch& last = m_batches.back();
        // Compare POD using memcmp
        if (last.shaderId == 1 && last.texId == texId && last.sortKey == sortKey &&
            raw_memcmp(&last.rr, &rr, sizeof(RRUniformState)) == 0) {
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
    b.rr = rr;
    m_batches.push_back(b);
    if (sortKey != 0) m_need_sort = true;
}

void Renderer2D::end() {
    flush();
}

// Optimized flush
void Renderer2D::flush() {
    if (m_ui_vtx.empty()) return;
    
    if (!m_batches.empty()) {
        unsigned int write = 0;
        const unsigned int count = m_batches.size();
        for (unsigned int i = 0; i < count; ++i) {
            if (m_batches[i].count != 0) {
                if (write != i) m_batches[write] = m_batches[i];
                ++write;
            }
        }
        if (write == 0u) {
            m_batches.clear();
            return;
        }
        if (write != count) m_batches.resize(write);
    }

    if (m_need_sort && m_batches.size() > 1u) {
        struct BatchLess {
            bool operator()(const Batch& a, const Batch& b) const {
                if (a.sortKey != b.sortKey) return a.sortKey < b.sortKey;
                if (a.shaderId != b.shaderId) return a.shaderId < b.shaderId;
                return a.texId < b.texId;
            }
        };
        shine::util::sort_inplace(m_batches.data(), m_batches.size(), BatchLess{});
    }

    CommandBuffer::Pass pass = g_cmd_buffer.begin_pass();

    // Upload & Bind
    pass.push(CMD_BIND_BUFFER, GL_ARRAY_BUFFER, m_vbo, 0, 0, 0, 0, 0);
    const int uploadCount = (int)m_ui_vtx.size();
    if (m_lastUploadCount > 0 && uploadCount <= m_lastUploadCount) {
        pass.push(CMD_BUFFER_SUB_DATA_F32, GL_ARRAY_BUFFER, 0, ptr_i32(m_ui_vtx.data()), uploadCount, 0, 0, 0);
    } else {
        pass.push(CMD_BUFFER_DATA_F32, GL_ARRAY_BUFFER, ptr_i32(m_ui_vtx.data()), uploadCount, GL_DYNAMIC_DRAW, 0, 0, 0);
        m_lastUploadCount = uploadCount;
    }
    pass.push(CMD_BIND_VAO, m_vao, 0, 0, 0, 0, 0, 0);

    // Render State
    int curShaderId = -1;
    int curTexId = -1;
    unsigned int setupMask = 0; // Bit 0: Tex, Bit 1: RR, Bit 2: Col
    RRUniformState lastRR;
    bool hasLastRR = false;
    
    // Cached Viewport Uniforms
    const int viewW_i = f2i((float)m_viewW);
    const int viewH_i = f2i((float)m_viewH);

    for (unsigned int i = 0; i < m_batches.size(); ++i) {
        const Batch& b = m_batches[i];

        // Shader Switch
        if (b.shaderId != curShaderId) {
            curShaderId = b.shaderId;
            int prog = (curShaderId == 0) ? m_prog_tex : ((curShaderId == 2) ? m_prog_col : m_prog_rr);
            pass.push(CMD_USE_PROGRAM, prog, 0, 0, 0, 0, 0, 0);

            // One-time Setup per shader
            if (curShaderId == 0) { // Tex
                if (!(setupMask & 1)) {
                    pass.push(CMD_SETUP_VIEW_SAMPLER2D, m_uViewSize, viewW_i, viewH_i, m_uTex, 0, 0, 0);
                    setupMask |= 1;
                }
            } else if (curShaderId == 2) { // Col
                if (!(setupMask & 4)) {
                    pass.push(CMD_UNIFORM2F, m_uCol_ViewSize, viewW_i, viewH_i, 0, 0, 0, 0);
                    setupMask |= 4;
                }
            } else { // RR
                // Always reset RR state on shader switch? Or just view?
                if (!(setupMask & 2)) {
                    pass.push(CMD_UNIFORM2F, m_uRR_ViewSize, viewW_i, viewH_i, 0, 0, 0, 0);
                    pass.push(CMD_UNIFORM1I, m_uRR_Tex, 0, 0, 0, 0, 0, 0); // Always slot 0
                    setupMask |= 2;
                }
            }
        }

        // Uniform Updates (RR only)
        if (curShaderId == 1) {
            updateRRUniforms(b.rr, lastRR, hasLastRR, pass);
        }

        // Texture Bind
        if (b.texId != curTexId) {
            curTexId = b.texId;
            if (curShaderId != 2 && b.texId != 0) {
                 pass.push(CMD_BIND_TEXTURE, GL_TEXTURE_2D, b.texId, 0, 0, 0, 0, 0);
            }
        }

        pass.push(CMD_DRAW_ARRAYS, GL_TRIANGLES, b.offset, b.count, 0, 0, 0, 0);
    }
    
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
    checkBatch(2, 0, firstVertex, 6, 0);

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
    
    checkBatch(0, texId, firstVertex, 6, 0);

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

void Renderer2D::drawRectColorSorted(float cx, float cy, float w, float h, float r, float g, float b, unsigned int sortKey) {
    int firstVertex = 0;
    float* d = allocVtx(6 * 5, &firstVertex);
    checkBatch(2, 0, firstVertex, 6, sortKey);

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

void Renderer2D::drawRectUVSorted(int texId, float cx, float cy, float w, float h, unsigned int sortKey) {
    int firstVertex = 0;
    float* d = allocVtx(6 * 5, &firstVertex);
    
    checkBatch(0, texId, firstVertex, 6, sortKey);

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

void Renderer2D::drawRoundRect(const Rect& rect, const RoundRectStyle& style) {
    int firstVertex = 0;
    float* d = allocVtx(6 * 5, &firstVertex);

    // Optimized struct initialization using memcpy/array copy logic if possible, 
    // or just cleaner assignments to avoid scattered stores.
    // Actually, let's just initialize directly.
    
    // Pre-calculate common floats
    const float w = rect.w;
    const float h = rect.h;
    const float inv_w = (w > 0.1f) ? (1.0f / w) : 0.0f;
    const float inv_h = (h > 0.1f) ? (1.0f / h) : 0.0f;
    const float inv_sum = (w + h > 0.1f) ? (2.0f / (w + h)) : 0.0f;
    
    RRUniformState rr;
    // Batch integer assignments
    int* pInt = (int*)&rr;
    // Layout: useTex, radX, radY, colorR...
    *pInt++ = (style.texId ? 1 : 0);
    *pInt++ = f2i(style.radius_px * inv_w);
    *pInt++ = f2i(style.radius_px * inv_h);
    
    // Fill Color
    *pInt++ = f2i(style.fill.r); *pInt++ = f2i(style.fill.g); *pInt++ = f2i(style.fill.b); *pInt++ = f2i(style.fill.a);
    
    // Tex Tint
    *pInt++ = f2i(style.texTint.r); *pInt++ = f2i(style.texTint.g); *pInt++ = f2i(style.texTint.b); *pInt++ = f2i(style.texTint.a);
    
    // Border Color
    *pInt++ = f2i(style.borderColor.r); *pInt++ = f2i(style.borderColor.g); *pInt++ = f2i(style.borderColor.b); *pInt++ = f2i(style.borderColor.a);
    
    // Border Width
    *pInt++ = f2i(style.border_px * inv_sum);
    
    // Shadow Color
    *pInt++ = f2i(style.shadowColor.r); *pInt++ = f2i(style.shadowColor.g); *pInt++ = f2i(style.shadowColor.b); *pInt++ = f2i(style.shadowColor.a);
    
    // Shadow Params
    *pInt++ = f2i(style.shadow_off_x * inv_w);
    *pInt++ = f2i(style.shadow_off_y * inv_h);
    *pInt++ = f2i(style.shadow_blur * inv_sum);
    *pInt++ = f2i(style.shadow_spread * inv_sum);

    // Check Batch Logic for RR
    checkBatchRR(style.texId, rr, firstVertex, 6, 0);

    // Fill vertices
    const float cx = rect.cx;
    const float cy = rect.cy;
    float x1 = cx - w * 0.5f;
    float y1 = cy - h * 0.5f;
    float x2 = cx + w * 0.5f;
    float y2 = cy + h * 0.5f;

    // Unrolled loop for quad (6 vertices)
    // 0: TL (0,0)
    d[0] = x1; d[1] = y1; d[2] = 0.0f; d[3] = 0.0f; d[4] = 0.0f;
    // 1: TR (1,0)
    d[5] = x2; d[6] = y1; d[7] = 1.0f; d[8] = 0.0f; d[9] = 0.0f;
    // 2: BL (0,1)
    d[10]= x1; d[11]= y2; d[12]= 0.0f; d[13]= 1.0f; d[14]= 0.0f;
    // 3: BL (0,1)
    d[15]= x1; d[16]= y2; d[17]= 0.0f; d[18]= 1.0f; d[19]= 0.0f;
    // 4: TR (1,0)
    d[20]= x2; d[21]= y1; d[22]= 1.0f; d[23]= 0.0f; d[24]= 0.0f;
    // 5: BR (1,1)
    d[25]= x2; d[26]= y2; d[27]= 1.0f; d[28]= 1.0f; d[29]= 0.0f;
}

void Renderer2D::updateRRUniforms(const RRUniformState& b, RRUniformState& last, bool& hasLastRR, CommandBuffer::Pass& pass) {
    if (hasLastRR) {
        // RRUniformState is POD, packed with ints.
        if (raw_memcmp(&b, &last, sizeof(RRUniformState)) == 0) return;
    }

    const unsigned int base = m_rr_blocks.size();
    m_rr_blocks.resize_uninitialized(base + 34u);
    int* out = m_rr_blocks.data() + base;
    // Locations
    out[0]  = m_uRR_Rad;
    out[1]  = m_uRR_UseTex;
    out[2]  = m_uRR_Color;
    out[3]  = m_uRR_TexTint;
    out[4]  = m_uRR_BorderColor;
    out[5]  = m_uRR_Border;
    out[6]  = m_uRR_ShadowColor;
    out[7]  = m_uRR_ShadowOff;
    out[8]  = m_uRR_ShadowBlur;
    out[9]  = m_uRR_ShadowSpread;
    // Values
    out[10] = b.radX;
    out[11] = b.radY;
    out[12] = b.useTex;
    out[13] = b.colorR;
    out[14] = b.colorG;
    out[15] = b.colorB;
    out[16] = b.colorA;
    out[17] = b.texTintR;
    out[18] = b.texTintG;
    out[19] = b.texTintB;
    out[20] = b.texTintA;
    out[21] = b.borderColorR;
    out[22] = b.borderColorG;
    out[23] = b.borderColorB;
    out[24] = b.borderColorA;
    out[25] = b.border;
    out[26] = b.shadowColorR;
    out[27] = b.shadowColorG;
    out[28] = b.shadowColorB;
    out[29] = b.shadowColorA;
    out[30] = b.shadowOffX;
    out[31] = b.shadowOffY;
    out[32] = b.shadowBlur;
    out[33] = b.shadowSpread;

    pass.push(CMD_UNIFORM_RR_BLOCK, ptr_i32(out), 0, 0, 0, 0, 0, 0);
    
    // Use raw memcpy to update last
    raw_memcpy(&last, &b, sizeof(RRUniformState));
    hasLastRR = true;
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
