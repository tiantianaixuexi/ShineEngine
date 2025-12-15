// Single demo: batch triangles rendered via WebGL ctx handles (Emscripten-style idea).
// - C++ generates lots of triangles (CPU-side) every frame
// - C++ calls JS WebGL wrappers (imports) to upload + draw
// - JS is a thin platform layer only (ctx table + WebGL calls)
//
// Exports:
//   init(tri_count)
//   on_resize(w,h)
//   frame(t_seconds)
//
// No libc / no emscripten.

using uintptr_t = __UINTPTR_TYPE__;

extern "C" {
int js_create_context(int canvas_id_ptr, int canvas_id_len); // returns ctxId
int js_create_texture_checker(int ctx, int size); // returns texId
void js_tex_load_url(int ctx, int url_ptr, int url_len, int reqId);
void js_tex_load_dataurl(int ctx, int data_ptr, int data_len, int reqId);
void js_tex_load_base64(int ctx, int mime_ptr, int mime_len, int b64_ptr, int b64_len, int reqId);
int  js_tex_load_url_sync(int ctx, int url_ptr, int url_len);           // returns texId if cached else 0
int  js_tex_load_dataurl_sync(int ctx, int data_ptr, int data_len);     // returns texId if cached else 0
int  js_tex_load_base64_sync(int ctx, int mime_ptr, int mime_len, int b64_ptr, int b64_len); // cached else 0
int  js_tex_get_wh(int ctx, int texId); // returns packed (w<<16)|h, 0 if unknown

int  gl_create_shader(int ctx, int type, int src_ptr, int src_len);
int  gl_create_program(int ctx, int vs, int fs);
void gl_use_program(int ctx, int prog);
int  gl_get_uniform_location(int ctx, int prog, int name_ptr, int name_len); // returns locId
void gl_uniform1i(int ctx, int loc, int v);
void gl_uniform1f(int ctx, int loc, float v);
void gl_uniform2f(int ctx, int loc, float x, float y);
void gl_uniform4f(int ctx, int loc, float x, float y, float z, float w);
int  gl_create_buffer(int ctx);
void gl_bind_buffer(int ctx, int target, int buf);
void gl_buffer_data_f32(int ctx, int target, int ptr, int float_count, int usage);
void gl_enable_attribs(int ctx);
void gl_viewport(int ctx, int x, int y, int w, int h);
void gl_clear_color(int ctx, float r, float g, float b, float a);
void gl_clear(int ctx, int mask);
void gl_active_texture(int ctx, int unit);
void gl_bind_texture(int ctx, int target, int tex);
void gl_draw_arrays(int ctx, int mode, int first, int count);
}

#include "logfmt.h"
#include "math/Matrix4.h"
#include "Container/SArray.h"
#include "ui/ui.h"
#include "ui/button.h"
#include "ui/image.h"
#include "util/wasm_compat.h"
#include "util/tex_loader.h"

using shine::wasm::ptr_i32;

static int g_ctx = 0;
static int g_prog = 0;
static int g_prog_tex = 0;
static int g_uTex = 0; // locId
static int g_prog_rr = 0;
static int g_uRR_Tex = 0;
static int g_uRR_UseTex = 0;
static int g_uRR_Color = 0;
static int g_uRR_Rad = 0;
static int g_uRR_TexTint = 0;
static int g_uRR_BorderColor = 0;
static int g_uRR_Border = 0;
static int g_uRR_ShadowColor = 0;
static int g_uRR_ShadowOff = 0;
static int g_uRR_ShadowBlur = 0;
static int g_uRR_ShadowSpread = 0;
static int g_vbo = 0;
static int g_w = 1;
static int g_h = 1;
static int g_tri_count = 0;
static shine::wasm::SArray<float> g_buf; // per-vertex: x,y,r,g,b (5 floats)

// Example: user-defined button behavior via inheritance.

static shine::ui::Button* g_btn = nullptr;

static shine::ui::Image g_img;
static shine::ui::Element* g_ui[8];
static int g_ui_count = 0;

static float g_ptr_x = 0.0f;
static float g_ptr_y = 0.0f;
static int g_ptr_down = 0;

// -------------------------------
// Async texture requests (callback mode) - factored to util/tex_loader.h
// -------------------------------
static shine::wasm::TexLoader g_tex;

extern "C" int ui_tex_request_url(const char* url, int urlLen, int* out_texId, int* out_w, int* out_h) {
  return g_tex.request_url(g_ctx, url, urlLen, out_texId, out_w, out_h);
}
extern "C" int ui_tex_request_dataurl(const char* dataurl, int dataLen, int* out_texId, int* out_w, int* out_h) {
  return g_tex.request_dataurl(g_ctx, dataurl, dataLen, out_texId, out_w, out_h);
}
extern "C" int ui_tex_request_base64(const char* mime, int mimeLen, const char* b64, int b64Len,
                                     int* out_texId, int* out_w, int* out_h) {
  return g_tex.request_base64(g_ctx, mime, mimeLen, b64, b64Len, out_texId, out_w, out_h);
}

extern "C" void on_tex_loaded(int reqId, int texId, int w, int h) { g_tex.on_loaded(reqId, texId, w, h); }
extern "C" void on_tex_failed(int reqId, int errCode) { (void)errCode; g_tex.on_failed(reqId); }



static void ensure_buffer(int tri_count) {
  // x,y,r,g,b per vertex; 3 vertices per tri
  const unsigned int floats_per_tri = 3u * 5u;
  const unsigned int total = (unsigned int)tri_count * floats_per_tri;
  g_buf = shine::wasm::SArray<float>(total);
}

static void update_vertices(float t) {
  if (g_tri_count <= 0 || g_buf.data() == nullptr) return;

  float aspect = (float)g_w / (float)g_h;
  float sx = 1.0f;
  if (shine::math::f_abs(aspect) > 0.00001f) sx = 1.0f / aspect;

  int n = g_tri_count;
  int grid = 1;
  while (grid * grid < n) grid++;

  float cell = 2.0f / (float)grid;
  float size = cell * 0.28f;

  float* outp = g_buf.data();
  for (int i = 0; i < n; ++i) {
    int gx = i % grid;
    int gy = i / grid;

    float cx = -1.0f + (gx + 0.5f) * cell;
    float cy = -1.0f + (gy + 0.5f) * cell;

    float dx = shine::math::tri_wave(t * 0.20f + (float)i * 0.017f) * (cell * 0.12f);
    float dy = shine::math::tri_wave(t * 0.17f + (float)i * 0.013f) * (cell * 0.12f);
    cx += dx;
    cy += dy;

    // Animated color (no sin/cos, no libm)
    float r = shine::math::tri01(t * 0.35f + (float)i * 0.031f);
    float g = shine::math::tri01(t * 0.29f + (float)i * 0.027f + 0.33f);
    float b = shine::math::tri01(t * 0.23f + (float)i * 0.019f + 0.66f);

    // Matrix demo:
    // M = Translate(cx,cy) * RotateZ(angle) * Scale(size*sx, size, 1)
    float angle = (t * 0.8f) + (float)i * 0.01f;
    Matrix4 M = Matrix4::multiply(
      Matrix4::translation(cx, cy, 0.0f),
      Matrix4::multiply(
        Matrix4::rotationZ(angle),
        Matrix4::scale(size * sx, size, 1.0f)
      )
    );

    float x0, y0, x1, y1, x2, y2;
    M.transformPoint2(0.0f,  1.0f, x0, y0);
    M.transformPoint2(-1.0f, -1.0f, x1, y1);
    M.transformPoint2(1.0f,  -1.0f, x2, y2);

    // v0
    *outp++ = x0; *outp++ = y0; *outp++ = r; *outp++ = g; *outp++ = b;
    // v1
    *outp++ = x1; *outp++ = y1; *outp++ = r; *outp++ = g; *outp++ = b;
    // v2
    *outp++ = x2; *outp++ = y2; *outp++ = r; *outp++ = g; *outp++ = b;
  }
}

static constexpr int GL_ARRAY_BUFFER = 0x8892;
static constexpr int GL_DYNAMIC_DRAW = 0x88E8;
static constexpr int GL_COLOR_BUFFER_BIT = 0x00004000;
static constexpr int GL_TRIANGLES = 0x0004;
static constexpr int GL_VERTEX_SHADER = 0x8B31;
static constexpr int GL_FRAGMENT_SHADER = 0x8B30;
static constexpr int GL_TEXTURE_2D = 0x0DE1;

static const char kCanvasId[] = "c";

static const char kVS[] =
  "attribute vec2 aPos;\n"
  "attribute vec3 aCol;\n"
  "varying vec3 vCol;\n"
  "void main(){ vCol = aCol; gl_Position = vec4(aPos,0.0,1.0); }\n";

static const char kFS[] =
  "precision mediump float;\n"
  "varying vec3 vCol;\n"
  "void main(){ gl_FragColor = vec4(vCol, 1.0); }\n";

static const char kVS_TEX[] =
  "attribute vec2 aPos;\n"
  "attribute vec3 aCol;\n" // aCol.xy used as UV
  "varying vec2 vUV;\n"
  "void main(){ vUV = aCol.xy; gl_Position = vec4(aPos,0.0,1.0); }\n";

static const char kFS_TEX[] =
  "precision mediump float;\n"
  "varying vec2 vUV;\n"
  "uniform sampler2D uTex;\n"
  "void main(){ gl_FragColor = texture2D(uTex, vUV); }\n";

static const char kU_TEX[] = "uTex";

// Rounded-rect UI shader (optional texture + AA).
static const char kVS_RR[] =
  "attribute vec2 aPos;\n"
  "attribute vec3 aCol;\n" // aCol.xy used as UV
  "varying vec2 vUV;\n"
  "void main(){ vUV = aCol.xy; gl_Position = vec4(aPos,0.0,1.0); }\n";

static const char kFS_RR[] =
  "#extension GL_OES_standard_derivatives : enable\n"
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
  "uniform vec2 uRad;\n"     // radius in uv units (x,y)
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
  "  float aa = fwidth(d);\n"
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

static void draw_rect_col(int ctxId, float cx, float cy, float w, float h, float r, float g, float b) {
  // two triangles, 6 verts, each: x,y,r,g,b
  float hx = w * 0.5f;
  float hy = h * 0.5f;
  float v[6 * 5] = {
    cx - hx, cy - hy, r, g, b,
    cx + hx, cy - hy, r, g, b,
    cx + hx, cy + hy, r, g, b,
    cx - hx, cy - hy, r, g, b,
    cx + hx, cy + hy, r, g, b,
    cx - hx, cy + hy, r, g, b,
  };
  gl_use_program(ctxId, g_prog);
  gl_bind_buffer(ctxId, GL_ARRAY_BUFFER, g_vbo);
  gl_buffer_data_f32(ctxId, GL_ARRAY_BUFFER, ptr_i32(v), 6 * 5, GL_DYNAMIC_DRAW);
  gl_draw_arrays(ctxId, GL_TRIANGLES, 0, 6);
}

static void draw_rect_uv(int ctxId, int texId, float cx, float cy, float w, float h) {
  float hx = w * 0.5f;
  float hy = h * 0.5f;
  // pack u,v into aCol.xy, keep aCol.z = 0
  float v[6 * 5] = {
    cx - hx, cy - hy, 0.0f, 0.0f, 0.0f,
    cx + hx, cy - hy, 1.0f, 0.0f, 0.0f,
    cx + hx, cy + hy, 1.0f, 1.0f, 0.0f,
    cx - hx, cy - hy, 0.0f, 0.0f, 0.0f,
    cx + hx, cy + hy, 1.0f, 1.0f, 0.0f,
    cx - hx, cy + hy, 0.0f, 1.0f, 0.0f,
  };
  gl_use_program(ctxId, g_prog_tex);
  gl_active_texture(ctxId, 0);
  gl_bind_texture(ctxId, GL_TEXTURE_2D, texId);
  gl_uniform1i(ctxId, g_uTex, 0);
  gl_bind_buffer(ctxId, GL_ARRAY_BUFFER, g_vbo);
  gl_buffer_data_f32(ctxId, GL_ARRAY_BUFFER, ptr_i32(v), 6 * 5, GL_DYNAMIC_DRAW);
  gl_draw_arrays(ctxId, GL_TRIANGLES, 0, 6);
}

// ui/ui.h expects these C symbols.
extern "C" void ui_draw_rect_col(int ctxId, float cx, float cy, float w, float h, float r, float g, float b) {
  draw_rect_col(ctxId, cx, cy, w, h, r, g, b);
}
extern "C" void ui_draw_rect_uv(int ctxId, float cx, float cy, float w, float h, int texId) {
  draw_rect_uv(ctxId, texId, cx, cy, w, h);
}
extern "C" void ui_draw_round_rect(int ctxId, float cx, float cy, float w, float h,
                                   float radius_px,
                                   float fill_r, float fill_g, float fill_b, float fill_a,
                                   int texId,
                                   float texTint_r, float texTint_g, float texTint_b, float texTint_a,
                                   float border_px,
                                   float border_r, float border_g, float border_b, float border_a,
                                   float shadow_off_px_x, float shadow_off_px_y,
                                   float shadow_blur_px, float shadow_spread_px,
                                   float shadow_r, float shadow_g, float shadow_b, float shadow_a) {
  // Geometry: rect with uv 0..1
  const float hx = w * 0.5f;
  const float hy = h * 0.5f;
  float v[6 * 5] = {
    cx - hx, cy - hy, 0.0f, 0.0f, 0.0f,
    cx + hx, cy - hy, 1.0f, 0.0f, 0.0f,
    cx + hx, cy + hy, 1.0f, 1.0f, 0.0f,
    cx - hx, cy - hy, 0.0f, 0.0f, 0.0f,
    cx + hx, cy + hy, 1.0f, 1.0f, 0.0f,
    cx - hx, cy + hy, 0.0f, 1.0f, 0.0f,
  };

  gl_use_program(ctxId, g_prog_rr);
  gl_uniform4f(ctxId, g_uRR_Color, fill_r, fill_g, fill_b, fill_a);
  gl_uniform4f(ctxId, g_uRR_TexTint, texTint_r, texTint_g, texTint_b, texTint_a);
  gl_uniform4f(ctxId, g_uRR_BorderColor, border_r, border_g, border_b, border_a);
  gl_uniform4f(ctxId, g_uRR_ShadowColor, shadow_r, shadow_g, shadow_b, shadow_a);

  // Convert radius_px to uv units for this rect.
  float pxW = w * (float)g_w * 0.5f;
  float pxH = h * (float)g_h * 0.5f;
  if (pxW < 1.0f) pxW = 1.0f;
  if (pxH < 1.0f) pxH = 1.0f;
  float rx = radius_px / pxW;
  float ry = radius_px / pxH;
  gl_uniform2f(ctxId, g_uRR_Rad, rx, ry);

  float minPx = (pxW < pxH) ? pxW : pxH;
  if (minPx < 1.0f) minPx = 1.0f;
  float bUv = border_px / minPx;
  gl_uniform1f(ctxId, g_uRR_Border, bUv);

  float offUx = shadow_off_px_x / pxW;
  float offUy = shadow_off_px_y / pxH;
  gl_uniform2f(ctxId, g_uRR_ShadowOff, offUx, offUy);

  float blurUv = shadow_blur_px / minPx;
  float spreadUv = shadow_spread_px / minPx;
  gl_uniform1f(ctxId, g_uRR_ShadowBlur, blurUv);
  gl_uniform1f(ctxId, g_uRR_ShadowSpread, spreadUv);

  if (texId != 0) {
    gl_active_texture(ctxId, 0);
    gl_bind_texture(ctxId, GL_TEXTURE_2D, texId);
    gl_uniform1i(ctxId, g_uRR_Tex, 0);
    gl_uniform1i(ctxId, g_uRR_UseTex, 1);
  } else {
    gl_uniform1i(ctxId, g_uRR_UseTex, 0);
  }

  gl_bind_buffer(ctxId, GL_ARRAY_BUFFER, g_vbo);
  gl_buffer_data_f32(ctxId, GL_ARRAY_BUFFER, ptr_i32(v), 6 * 5, GL_DYNAMIC_DRAW);
  gl_draw_arrays(ctxId, GL_TRIANGLES, 0, 6);
}
static void ui_add(shine::ui::Element* e) {
  if (g_ui_count >= (int)(sizeof(g_ui) / sizeof(g_ui[0]))) return;
  g_ui[g_ui_count++] = e;
}

static void ui_render_all() {
  for (int i = 0; i < g_ui_count; ++i) {
    shine::ui::Element* e = g_ui[i];
    if (!e || !e->visible) continue;
    e->render(g_ctx);
  }
}

static void ui_resize_all(int w, int h) {
  for (int i = 0; i < g_ui_count; ++i) {
    shine::ui::Element* e = g_ui[i];
    if (!e || !e->visible) continue;
    e->onResize(w, h);
  }
}

extern "C" void init(int tri_count) {
  // Get context handle from JS
  g_ctx = js_create_context(ptr_i32(kCanvasId), (int)(sizeof(kCanvasId) - 1));
  LOG("ctxId", g_ctx);

  if (tri_count < 0) tri_count = 0;
  if (tri_count > 10000) tri_count = 10000;
  ensure_buffer(tri_count);
  g_tri_count = tri_count;
  LOG("tri_count", g_tri_count);

  int vs = gl_create_shader(g_ctx, GL_VERTEX_SHADER, ptr_i32(kVS), (int)(sizeof(kVS) - 1));
  int fs = gl_create_shader(g_ctx, GL_FRAGMENT_SHADER, ptr_i32(kFS), (int)(sizeof(kFS) - 1));
  g_prog = gl_create_program(g_ctx, vs, fs);
  gl_use_program(g_ctx, g_prog);

  // UI texture program
  int uvs = gl_create_shader(g_ctx, GL_VERTEX_SHADER, ptr_i32(kVS_TEX), (int)(sizeof(kVS_TEX) - 1));
  int ufs = gl_create_shader(g_ctx, GL_FRAGMENT_SHADER, ptr_i32(kFS_TEX), (int)(sizeof(kFS_TEX) - 1));
  g_prog_tex = gl_create_program(g_ctx, uvs, ufs);
  g_uTex = gl_get_uniform_location(g_ctx, g_prog_tex, ptr_i32(kU_TEX), (int)(sizeof(kU_TEX) - 1));

  // UI rounded-rect program
  int rvs = gl_create_shader(g_ctx, GL_VERTEX_SHADER, ptr_i32(kVS_RR), (int)(sizeof(kVS_RR) - 1));
  int rfs = gl_create_shader(g_ctx, GL_FRAGMENT_SHADER, ptr_i32(kFS_RR), (int)(sizeof(kFS_RR) - 1));
  g_prog_rr = gl_create_program(g_ctx, rvs, rfs);
  g_uRR_Tex = gl_get_uniform_location(g_ctx, g_prog_rr, ptr_i32(kU_RR_TEX), (int)(sizeof(kU_RR_TEX) - 1));
  g_uRR_UseTex = gl_get_uniform_location(g_ctx, g_prog_rr, ptr_i32(kU_RR_USETEX), (int)(sizeof(kU_RR_USETEX) - 1));
  g_uRR_Color = gl_get_uniform_location(g_ctx, g_prog_rr, ptr_i32(kU_RR_COLOR), (int)(sizeof(kU_RR_COLOR) - 1));
  g_uRR_Rad = gl_get_uniform_location(g_ctx, g_prog_rr, ptr_i32(kU_RR_RAD), (int)(sizeof(kU_RR_RAD) - 1));
  g_uRR_TexTint = gl_get_uniform_location(g_ctx, g_prog_rr, ptr_i32(kU_RR_TEXTINT), (int)(sizeof(kU_RR_TEXTINT) - 1));
  g_uRR_BorderColor = gl_get_uniform_location(g_ctx, g_prog_rr, ptr_i32(kU_RR_BORDERCOLOR), (int)(sizeof(kU_RR_BORDERCOLOR) - 1));
  g_uRR_Border = gl_get_uniform_location(g_ctx, g_prog_rr, ptr_i32(kU_RR_BORDER), (int)(sizeof(kU_RR_BORDER) - 1));
  g_uRR_ShadowColor = gl_get_uniform_location(g_ctx, g_prog_rr, ptr_i32(kU_RR_SHADOWCOLOR), (int)(sizeof(kU_RR_SHADOWCOLOR) - 1));
  g_uRR_ShadowOff = gl_get_uniform_location(g_ctx, g_prog_rr, ptr_i32(kU_RR_SHADOWOFF), (int)(sizeof(kU_RR_SHADOWOFF) - 1));
  g_uRR_ShadowBlur = gl_get_uniform_location(g_ctx, g_prog_rr, ptr_i32(kU_RR_SHADOWBLUR), (int)(sizeof(kU_RR_SHADOWBLUR) - 1));
  g_uRR_ShadowSpread = gl_get_uniform_location(g_ctx, g_prog_rr, ptr_i32(kU_RR_SHADOWSPREAD), (int)(sizeof(kU_RR_SHADOWSPREAD) - 1));

  g_vbo = gl_create_buffer(g_ctx);
  gl_bind_buffer(g_ctx, GL_ARRAY_BUFFER, g_vbo);
  update_vertices(0.0f);
  gl_buffer_data_f32(g_ctx, GL_ARRAY_BUFFER, ptr_i32(g_buf.data()), g_tri_count * 3 * 5, GL_DYNAMIC_DRAW);
  gl_enable_attribs(g_ctx);

  // init UI demo elements
  g_ui_count = 0;

  // Init button (user code may allocate its style here).
  g_btn = shine::ui::Button::create();
  g_btn->bindOnClick(
      [](shine::ui::Button* self, void* /*user*/) {
        LOG("button clicked");
        g_btn->setPosPx(150.0f, 12.0f);
        // Browser URL (JS will normalize/encode it for fetch).
        g_btn->setBgUrl("asset/金币.png");
        // If your style tint alpha is < 1, textures can look faint.
        // if (g_btn->style) g_btn->style->bg_tex_tint = {1.0f, 1.0f, 1.0f, 1.0f};
        (void)self;
      },
      nullptr
  );

  // Responsive layout: bottom-left, 12px padding, 220x64 px
  g_btn->setLayoutPx(0.0f, 0.0f, 12.0f, 12.0f, 220.0f, 64.0f);
  ui_add(g_btn);

  // Responsive layout: bottom-right, 12px padding, 220x160 px
  // (offsetX is negative to move left from the right edge)
  g_img.setLayoutPx(1.0f, -1.0f, -12.0f, 12.0f, 220.0f, 160.0f);
  g_img.texId = js_create_texture_checker(g_ctx, 64);
  ui_add(&g_img);

  // Example (optional): async texture load + callback + cache lives in JS.
  // You can call these from anywhere (url or data:url). Here we only demonstrate in DEBUG.
#ifdef DEBUG
  static const char kTinyPng[] =
    "data:image/png;base64,"
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8"
    "/w8AAn8B9p9pYwAAAABJRU5ErkJggg==";
  static const char kTinyB64[] =
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8"
    "/w8AAn8B9p9pYwAAAABJRU5ErkJggg==";
  static const char kMimePng[] = "image/png";
  if (g_btn && g_btn->style) {
    ui_tex_request_dataurl(kTinyPng, (int)(sizeof(kTinyPng) - 1), &g_btn->style->bg_texId, nullptr, nullptr);
    // Also demonstrate raw base64 API (no data: prefix).
    ui_tex_request_base64(kMimePng, (int)(sizeof(kMimePng) - 1), kTinyB64, (int)(sizeof(kTinyB64) - 1),
                          &g_btn->style->bg_texId, nullptr, nullptr);
  }
  ui_tex_request_dataurl(kTinyPng, (int)(sizeof(kTinyPng) - 1), &g_img.texId, &g_img.texW, &g_img.texH);
#endif
}

extern "C" void on_resize(int w, int h) {
  if (w < 1) w = 1;
  if (h < 1) h = 1;
  g_w = w;
  g_h = h;

  gl_viewport(g_ctx, 0, 0, w, h);

  // UI should adapt to viewport changes too.
  ui_resize_all(w, h);

  // positions are updated in frame(t)
}

extern "C" void frame(float t) {
  update_vertices(t);
  gl_clear_color(g_ctx, 0.07f, 0.07f, 0.07f, 1.0f);
  gl_clear(g_ctx, GL_COLOR_BUFFER_BIT);
  gl_use_program(g_ctx, g_prog);
  gl_bind_buffer(g_ctx, GL_ARRAY_BUFFER, g_vbo);
  gl_buffer_data_f32(g_ctx, GL_ARRAY_BUFFER, ptr_i32(g_buf.data()), g_tri_count * 3 * 5, GL_DYNAMIC_DRAW);
  gl_draw_arrays(g_ctx, GL_TRIANGLES, 0, g_tri_count * 3);

  // UI on top
  ui_render_all();
}

extern "C" void ui_pointer(float x_ndc, float y_ndc, int isDown) {
  g_ptr_x = x_ndc;
  g_ptr_y = y_ndc;
  g_ptr_down = isDown ? 1 : 0;
  for (int i = 0; i < g_ui_count; ++i) {
    shine::ui::Element* e = g_ui[i];
    if (!e || !e->visible) continue;
    e->pointer(x_ndc, y_ndc, g_ptr_down);
  }
}

