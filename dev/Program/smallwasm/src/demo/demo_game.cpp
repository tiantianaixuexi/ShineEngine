#include "demo_game.h"
#include "../graphics/gl_api.h"
#include "../graphics/wasm_command_buffer.h"
#include "../util/wasm_compat.h"
#include "../game/component.h"
#include "../game/transform.h"
#include "../game/sprite_renderer.h"
#include "../game/mesh_renderer_3d.h"
#include "../util/math_def.h" // Added for shine::math

#include "../ui/button.h"
#include "../ui/image.h"
#include "../ui/ui_manager.h" // Added UIManager

#include "../logfmt.h"
#include "../renderer/renderer_3d.h"
#include "../renderer/pass/base_pass.h"


using namespace shine::graphics;

DemoGame* g_demo_game = nullptr;
// g_demo_ui_list removed

// ----------------------------------------------------------------------------
// Demo Components
// ----------------------------------------------------------------------------

struct PulseColor final : public shine::game::Component {

    shine::game::SpriteRenderer* sr = nullptr;
    float base = 0.25f;
    explicit PulseColor(shine::game::SpriteRenderer* target) : sr(target) {
        setTypeId<PulseColor>();
    }
    void onUpdate(float t) override {
        if (sr) {
            float v = base + 0.4f * sin(t * 15.0f);
            sr->g = v;
            sr->b = v;
        }
    }
};

struct KillOnClick final : public shine::game::Component {
    KillOnClick() { setTypeId<KillOnClick>(); }
    void onPointer(float x, float y, int isDown) override {
        if (!isDown) return;
        if (!node) return;
        auto* tr = node->getComponent<shine::game::Transform>();
        if (!tr) return;

        float cx, cy;
        tr->worldXY(cx, cy);
        const float w = tr->w;
        const float h = tr->h;

        if (x < cx - w * 0.5f || x > cx + w * 0.5f) return;
        if (y < cy - h * 0.5f || y > cy + h * 0.5f) return;


        node->markPendingKill();
    }
};

// ----------------------------------------------------------------------------
// Shaders for raw demo
// ----------------------------------------------------------------------------

static SHINE_CONSTINIT const char kVS[] =
  "#version 300 es\n"
  "precision mediump float;in vec2 aPos;in vec3 aCol;out vec3 vCol;void main(){vCol=aCol;gl_Position=vec4(aPos,0.0,1.0);}";

static SHINE_CONSTINIT const char kFS[] =
  "#version 300 es\n"
  "precision mediump float;in vec3 vCol;out vec4 outColor;void main(){outColor=vec4(vCol,1.0);}";

static SHINE_CONSTINIT const char kVS_INST[] =
  "#version 300 es\n"
  "precision mediump float;in vec2 aPos;in vec3 aCol;in vec3 aOffsetScale;in vec3 aICol;out vec3 vCol;"
  "void main(){vec2 pos=aOffsetScale.xy+aPos*aOffsetScale.z;gl_Position=vec4(pos,0.0,1.0);vCol=aICol;}";

// ----------------------------------------------------------------------------
// DemoGame Implementation
// ----------------------------------------------------------------------------

static void demo_on_mode_click(shine::ui::Button* w) {
  (void)w;
  if (g_demo_game) {
      g_demo_game->render_mode = (g_demo_game->render_mode == 0) ? 1 : 0;
      LOG("render_mode", g_demo_game->render_mode);
  }
}

static void demo_rc_draw_rect_col(void* /*user*/, const shine::game::Rect& rect, const shine::game::Color4& color) {
    RENDERER_2D.drawRectColor(rect.cx, rect.cy, rect.w, rect.h, color.r, color.g, color.b);
}

static void demo_rc_draw_rect_tex(void* /*user*/, int texId, const shine::game::Rect& rect) {
    RENDERER_2D.drawRectUV(texId, rect.cx, rect.cy, rect.w, rect.h);
}

static void demo_rc_draw_rect_col_ex(void* /*user*/, const shine::game::Rect& rect, const shine::game::Color4& color, unsigned int sortKey) {
    RENDERER_2D.drawRectColorSorted(rect.cx, rect.cy, rect.w, rect.h, color.r, color.g, color.b, sortKey);
}

static void demo_rc_draw_rect_tex_ex(void* /*user*/, int texId, const shine::game::Rect& rect, unsigned int sortKey) {
    RENDERER_2D.drawRectUVSorted(texId, rect.cx, rect.cy, rect.w, rect.h, sortKey);
}

// UI list wrapper for legacy ui_add - REMOVED
// static shine::wasm::SVector<shine::ui::Element*> g_demo_ui_list;
// static void ui_add(shine::ui::Element* e) {
//    g_demo_ui_list.push_back(e);
// }

static void demo_on_btn_click(shine::ui::Button*) { LOG("button clicked"); }
static void demo_on_btn_hover(shine::ui::Button*) { LOG("button Hover"); }
static void demo_on_btn_unhover(shine::ui::Button*) { LOG("button UnHover"); }

static __attribute__((noinline)) void demo_init_shaders(DemoGame* g, int ctx) {
    g->prog = gl_create_program_from_source(ctx, kVS, kFS);
    g->vbo = gl_create_buffer(ctx);
    g->vao_basic = gl_create_vertex_array(ctx);
    
    gl_bind_vertex_array(ctx, g->vao_basic);
    gl_setup_attribs_basic(ctx, g->vbo);
    gl_bind_vertex_array(ctx, 0);

    g->prog_inst = gl_create_program_instanced(ctx, 
        gl_create_shader(ctx, GL_VERTEX_SHADER, (int)kVS_INST, sizeof(kVS_INST)-1),
        gl_create_shader(ctx, GL_FRAGMENT_SHADER, (int)kFS, sizeof(kFS)-1)
    );
    static SHINE_CONSTINIT const float q[] = {
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 0.0f, 0.0f
    };
    g->vbo_inst_base = gl_create_buffer(ctx);
    gl_bind_buffer(ctx, GL_ARRAY_BUFFER, g->vbo_inst_base);
    gl_buffer_data_f32(ctx, GL_ARRAY_BUFFER, ptr_i32(q), 30, GL_DYNAMIC_DRAW);

    g->vbo_inst_data = gl_create_buffer(ctx);
    g->vao_inst = gl_create_vertex_array(ctx);
    
    gl_bind_vertex_array(ctx, g->vao_inst);
    gl_setup_attribs_instanced(ctx, g->vbo_inst_base, g->vbo_inst_data);
    gl_bind_vertex_array(ctx, 0);
}

static __attribute__((noinline)) void demo_init_scene(DemoGame* g, int ctx) {
    g->player = g->scene.root.addChildNode<shine::game::Node>("Player");
    g->weapon = g->player->addChildNode<shine::game::Node>("Weapon");

    auto* tPlayer = g->player->addComponent<shine::game::Transform>();
    tPlayer->setPosition(0.0f, 0.0f);
    tPlayer->setSize(0.35f, 0.35f);
    auto* sPlayer = g->player->addComponent<shine::game::SpriteRenderer>();
    sPlayer->texId = js_create_texture_checker(ctx, 64);

    auto* tWeapon = g->weapon->addComponent<shine::game::Transform>();
    tWeapon->setPosition(0.45f, 0.05f);
    tWeapon->setSize(0.22f, 0.12f);
    auto* sWeapon = g->weapon->addComponent<shine::game::SpriteRenderer>();
    sWeapon->texId = 0; 
    sWeapon->r = 0.9f; sWeapon->g = 0.2f; sWeapon->b = 0.2f;

    g->weapon->addComponent<KillOnClick>();
}

static __attribute__((noinline)) void demo_init_ui(DemoGame* g, int ctx) {
    shine::ui::UIManager::instance().clear();

    g->btn = shine::ui::Button::create();
    g->btn->bindOnClick(demo_on_btn_click);
    g->btn->bindHoverEvent(demo_on_btn_hover);
    g->btn->bindUnHoverEvent(demo_on_btn_unhover);
    g->btn->setBgUrl("../asset/金币.png");
    g->btn->setAlignment(0.5f, 0.5f);
    g->btn->setLayoutRel(0.5f, 0.5f, 0.0f, 0.0f, 0.18f, 0.09f);
    g->btn->setLayoutPx(0.5f,0.5f,-50.f,50.f,100.f,100.f);
    shine::ui::UIManager::instance().add(g->btn);

    g->btn_mode = shine::ui::Button::create();
    g->btn_mode->bindOnClick(demo_on_mode_click);
    g->btn_mode->setLayoutRel(0.0f, 0.0f, 12.0f, 12.0f, 0.20f, 0.08f);
    shine::ui::UIManager::instance().add(g->btn_mode);

    g->img = new shine::ui::Image();
    g->img->setAlignment(1.0f, 1.0f);
    g->img->setLayoutRel(1.0f, 1.0f, -12.0f, -12.0f, 0.30f, 0.22f);
    g->img->texId = js_create_texture_checker(ctx, 64);
    shine::ui::UIManager::instance().add(g->img);
}

void DemoGame::onInit() {

    g_demo_game = this;
    int ctx = SHINE_ENGINE.getCtx();

    rc.user = nullptr;
    rc.drawRectCol = demo_rc_draw_rect_col;
    rc.drawRectTex = demo_rc_draw_rect_tex;
    rc.drawRectColEx = demo_rc_draw_rect_col_ex;
    rc.drawRectTexEx = demo_rc_draw_rect_tex_ex;
    pipeline.add_default_passes();

    demo_init_shaders(this, ctx);
    ensure_buffer(1500); // Default buffer for raw demo
    ensure_instanced(500); // default
    demo_init_scene(this, ctx);
    demo_init_ui(this, ctx);
}

void DemoGame::onResize(int w, int h) {
    (void)w;
    (void)h;

    //LOG2("Resize:", w, h);
}

void DemoGame::onUpdate(float t) {
    scene.update(t);
    if (++gc_frame >= gc_interval) {
        gc_frame = 0;
        scene.collectGarbage();
    }
}

void DemoGame::onRender(float t) {
    const shine::renderer::RenderPass* passes = pipeline.data();
    const unsigned int pass_count = pipeline.count();
    int ctx = SHINE_ENGINE.getCtx();
    shine::renderer::Renderer3D::instance().begin_frame();
    shine::renderer::Renderer3D::instance().ensure_frame_ubo(ctx, 0);

    // Per-frame 3D uniforms (identity view/proj for now).
    static SHINE_CONSTINIT const float kIdentity[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    shine::renderer::Renderer3D::instance().set_frame_matrices(kIdentity, kIdentity);
    shine::renderer::Renderer3D::instance().set_frame_light({0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f});

    shine::renderer::RunDemoPasses(*this, passes, pass_count, t, ctx);
}

void DemoGame::onPointer(float x, float y, int isDown) {
    scene.pointer(x, y, isDown);
    
    // Convert NDC to Pixels for UI
    float half_w = 0.f;
    float half_h = 0.f;

    SHINE_ENGINE.getHalf(half_w, half_h);


    float px = (x + 1.0f) * half_w;
    float py = (1.0f - y) * half_h;


    // UI pointer
    shine::ui::UIManager::instance().onPointer(px, py, isDown);
}

void DemoGame::ensure_buffer(int count) {
    // x,y,r,g,b per vertex; 3 vertices per tri
    //const unsigned int floats_per_tri = 3u * 5u;
    const unsigned int total = 15u * count;
    buf = FLOAT_ARRAY(total);
    tri_count = count;
}

void DemoGame::ensure_instanced(int count) {
    if (count < 1) count = 1;
    if (count > 20000) count = 20000;
    inst_count = count;
    inst = FLOAT_ARRAY((unsigned int)count * 6u);
}

void DemoGame::update_vertices(float t) {
    // Local copy to prevent aliasing/reloading
    const int count = tri_count;
    if (count <= 0 || !buf.data()) return;

 
    const float aspect = SHINE_ENGINE.aspect; 
    
    float sx = 1.0f;
    if (abs(aspect) > 0.00001f) sx = 1.0f / aspect;

    int grid = 1;
    while (grid * grid < count) grid++;

    float cell = 2.0f / (float)grid;
    float size = cell * 0.28f;
    const float inv_grid = 1.0f / (float)grid;
    const float b = 0.5f + 0.5f * sin(t);
    const float t_base = t * 0.1f;

    float* outp = buf.data();
    int remaining = count;
    
    float base_pos_off = -1.0f + 0.5f * cell;
    
    // Hoist variables to avoid repeated stack allocation
    float cx, cy, r, g;

    // Strength reduction state
    float cx_linear_base = base_pos_off;
    float cos_arg_base = t_base;
    float r_val_base = 0.0f;

    for (int gy = 0; gy < grid && remaining > 0; ++gy) {
        // Optimize: Calculate row limit to avoid inner loop branch
        // Compiler needs to see this is strictly <= grid
        int row_limit = remaining;
        if (row_limit > grid) row_limit = grid;

        // Hoist row-invariant calculations
        float gy_f = (float)gy;
        float cy_base_row = base_pos_off + gy_f * cell;
        // Hoist sin calculation (depends on gy)
        const float cx_sin_offset = sin(t_base + gy_f * 0.1f) * 0.05f;
        g = gy_f * inv_grid;

        // Strength reduction for inner loop
        float cx_linear = cx_linear_base;
        float cos_arg = cos_arg_base;
        float r_val = r_val_base;
        
        for (int gx = 0; gx < row_limit; ++gx) {
            cx = cx_linear + cx_sin_offset;
            // Cos depends on gx, but we use incremental arg
            cy = cy_base_row + cos(cos_arg) * 0.05f;
            r = r_val;
            
            float px0 = cx * sx, px1 = (cx - size) * sx, px2 = (cx + size) * sx;
            float py0 = cy + size, py1 = cy - size, py2 = cy - size;
            
            *outp++ = px0; *outp++ = py0; *outp++ = r; *outp++ = g; *outp++ = b;
            *outp++ = px1; *outp++ = py1; *outp++ = r; *outp++ = g; *outp++ = b;
            *outp++ = px2; *outp++ = py2; *outp++ = r; *outp++ = g; *outp++ = b;

            // Advance linear/incremental values
            cx_linear += cell;
            r_val += inv_grid;
            cos_arg += 0.1f;
        }

        remaining -= row_limit;
    }
}

void DemoGame::update_instances(float t) {
    if (inst_count <= 0 || !inst.data()) return;
    int n = inst_count;
    int grid = 1;
    while (1 < n) grid++;
    float cell = 2.0f / (float)grid;
    float baseScale = cell * 0.35f;
    const float offset_scale = cell * 0.18f;
    const float scale_base = baseScale * 0.75f;
    const float scale_var = baseScale * 0.35f;
    
    // Precompute constants
    const float k0 = 0.017f;
    const float k1 = 0.013f;
    const float k2 = 0.011f;
    const float k3 = 0.031f;
    const float k4 = 0.027f;
    const float k5 = 0.019f;

    float* outp = inst.data();
    int i = 0;
    for (int gy = 0; gy < grid && i < n; ++gy) {
        float cy = -1.0f + (gy + 0.5f) * cell;
        for (int gx = 0; gx < grid && i < n; ++gx, ++i) {
            float fi = (float)i;
            float cx = -1.0f + (gx + 0.5f) * cell;
            
            float dx = sin(t + fi * k0) * offset_scale;
            float dy = sin(t + fi * k1) * offset_scale;
            float sc = scale_base + scale_var * (0.5f + 0.5f * sin(t + fi * k2));

            float r = 0.5f + 0.5f * sin(t + fi * k3);
            float g = 0.5f + 0.5f * sin(t + fi * k4 + 2.0f);
            float b = 0.5f + 0.5f * sin(t + fi * k5 + 4.0f);

            *outp++ = cx + dx;
            *outp++ = cy + dy;
            *outp++ = sc;
            *outp++ = r;
            *outp++ = g;
            *outp++ = b;
        }
    }
}

#include "../graphics/texture_manager.h"

// Global Texture callbacks forwarding
extern "C" void on_tex_loaded(int reqId, int texId, int w, int h) { 
    shine::graphics::TextureManager::instance().on_loaded(reqId, texId, w, h);
}
extern "C" void on_tex_failed(int reqId, int errCode) { 
    (void)errCode;
    shine::graphics::TextureManager::instance().on_failed(reqId);
}

// Factory implementation
Game* CreateGame() {
    return new DemoGame();
}
