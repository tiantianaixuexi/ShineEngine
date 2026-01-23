#include "demo_game.h"
#include "../graphics/gl_api.h"
#include "../graphics/wasm_command_buffer.h"
#include "../util/wasm_compat.h"
#include "../game/component.h"
#include "../game/transform.h"
#include "../game/sprite_renderer.h"
#include "../util/math_def.h" // Added for shine::math

#include "../ui/button.h"
#include "../ui/image.h"
#include "../ui/ui_manager.h" // Added UIManager

#include "../logfmt.h"


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
            float v = base + 0.4f * shine::math::sin(t * 15.0f);
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

static const char kVS[] =
  "#version 300 es\n"
  "precision mediump float;in vec2 aPos;in vec3 aCol;out vec3 vCol;void main(){vCol=aCol;gl_Position=vec4(aPos,0.0,1.0);}";

static const char kFS[] =
  "#version 300 es\n"
  "precision mediump float;in vec3 vCol;out vec4 outColor;void main(){outColor=vec4(vCol,1.0);}";

static const char kVS_INST[] =
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

static void demo_rc_draw_rect_col(void* /*user*/, float cx, float cy, float w, float h, float r, float g, float b) {
    RENDERER_2D.drawRectColor(cx, cy, w, h, r, g, b);
}

static void demo_rc_draw_rect_tex(void* /*user*/, int texId, float cx, float cy, float w, float h) {
    RENDERER_2D.drawRectUV(texId, cx, cy, w, h);
}

// UI list wrapper for legacy ui_add - REMOVED
// static shine::wasm::SVector<shine::ui::Element*> g_demo_ui_list;
// static void ui_add(shine::ui::Element* e) {
//    g_demo_ui_list.push_back(e);
// }

void DemoGame::onInit() {

    g_demo_game = this;
    int ctx = SHINE_ENGINE.getCtx();

    rc.user = nullptr;
    rc.drawRectCol = demo_rc_draw_rect_col;
    rc.drawRectTex = demo_rc_draw_rect_tex;

    // Init raw shaders
    prog = gl_create_program_from_source(ctx, kVS, kFS);
    vbo = gl_create_buffer(ctx);
    vao_basic = gl_create_vertex_array(ctx);
    
    
    // Bind VAO before setting up attribs!
    gl_bind_vertex_array(ctx, vao_basic);
    gl_setup_attribs_basic(ctx, vbo);
    gl_bind_vertex_array(ctx, 0);

    // Init instanced shaders
    prog_inst = gl_create_program_instanced(ctx, 
        gl_create_shader(ctx, GL_VERTEX_SHADER, (int)kVS_INST, sizeof(kVS_INST)-1),
        gl_create_shader(ctx, GL_FRAGMENT_SHADER, (int)kFS, sizeof(kFS)-1)
    );
    // Base quad for instances
    static const float q[] = {
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 0.0f, 0.0f
    };
    vbo_inst_base = gl_create_buffer(ctx);
    gl_bind_buffer(ctx, GL_ARRAY_BUFFER, vbo_inst_base);
    gl_buffer_data_f32(ctx, GL_ARRAY_BUFFER, ptr_i32(q), 30, GL_DYNAMIC_DRAW);

    vbo_inst_data = gl_create_buffer(ctx);
    vao_inst = gl_create_vertex_array(ctx);
    
    gl_bind_vertex_array(ctx, vao_inst);
    gl_setup_attribs_instanced(ctx, vbo_inst_base, vbo_inst_data);
    gl_bind_vertex_array(ctx, 0);

    ensure_buffer(1500); // Default buffer for raw demo
    ensure_instanced(500); // default

    // Scene setup
    // player = scene.createNode("Player");
    // Scene helper missing, using root directly.
    player = scene.root.addChildNode<shine::game::Node>("Player");
    weapon = player->addChildNode<shine::game::Node>("Weapon");

    auto* tPlayer = player->addComponent<shine::game::Transform>();
    tPlayer->x = 0.0f; tPlayer->y = 0.0f; tPlayer->w = 0.35f; tPlayer->h = 0.35f;
    auto* sPlayer = player->addComponent<shine::game::SpriteRenderer>();
    sPlayer->texId = js_create_texture_checker(ctx, 64);

    auto* tWeapon = weapon->addComponent<shine::game::Transform>();
    tWeapon->x = 0.45f; tWeapon->y = 0.05f; tWeapon->w = 0.22f; tWeapon->h = 0.12f;
    auto* sWeapon = weapon->addComponent<shine::game::SpriteRenderer>();
    sWeapon->texId = 0; 
    sWeapon->r = 0.9f; sWeapon->g = 0.2f; sWeapon->b = 0.2f;

    sWeapon->attachChild(new PulseColor(sWeapon));
    weapon->addComponent<KillOnClick>();

    // UI setup
    shine::ui::UIManager::instance().clear();

    btn = shine::ui::Button::create();
    btn->bindOnClick([](shine::ui::Button*){ LOG("button clicked"); });
    btn->bindHoverEvent([](shine::ui::Button*) { LOG("button Hover"); });
    btn->bindUnHoverEvent([](shine::ui::Button*) { LOG("button UnHover"); });
    btn->setBgUrl("../asset/金币.png");
    btn->setAlignment(0.5f, 0.5f);
    btn->setLayoutRel(0.5f, 0.5f, 0.0f, 0.0f, 0.18f, 0.09f);
    btn->setLayoutPx(0.5f,0.5f,-50.f,50.f,100.f,100.f);
    shine::ui::UIManager::instance().add(btn);

    btn_mode = shine::ui::Button::create();
    btn_mode->bindOnClick(demo_on_mode_click);
    btn_mode->setLayoutRel(0.0f, 0.0f, 12.0f, 12.0f, 0.20f, 0.08f);
    shine::ui::UIManager::instance().add(btn_mode);

    img = new shine::ui::Image();
    img->setAlignment(1.0f, 1.0f);
    img->setLayoutRel(1.0f, 1.0f, -12.0f, -12.0f, 0.30f, 0.22f);
    img->texId = js_create_texture_checker(ctx, 64);
    shine::ui::UIManager::instance().add(img);

}

void DemoGame::onResize(int w, int h) {
    (void)w;
    (void)h;

    //LOG2("Resize:", w, h);
}

void DemoGame::onUpdate(float t) {
    scene.update(t);
    scene.collectGarbage();
}

void DemoGame::onRender(float t) {
    // 1. Draw raw demo stuff (Triangles / Instances)
    if (render_mode == 0) {
        update_vertices(t);
        if (tri_count > 0 && buf.data()) {
            cmd_push(CMD_BIND_BUFFER, GL_ARRAY_BUFFER, vbo, 0, 0, 0, 0, 0);
            cmd_push(CMD_BUFFER_DATA_F32, GL_ARRAY_BUFFER, ptr_i32(buf.data()), tri_count * 3 * 5, GL_DYNAMIC_DRAW, 0, 0, 0);
            cmd_push(CMD_BIND_VAO, vao_basic, 0, 0, 0, 0, 0, 0);
            cmd_push(CMD_USE_PROGRAM, prog, 0, 0, 0, 0, 0, 0);
            cmd_push(CMD_DRAW_ARRAYS, GL_TRIANGLES, 0, tri_count * 3, 0, 0, 0, 0);
        }
    } else {
        update_instances(t);
        if (inst_count > 0 && inst.data()) {
            cmd_push(CMD_BIND_BUFFER, GL_ARRAY_BUFFER, vbo_inst_data, 0, 0, 0, 0, 0);
            cmd_push(CMD_BUFFER_DATA_F32, GL_ARRAY_BUFFER, ptr_i32(inst.data()), inst_count * 6, GL_DYNAMIC_DRAW, 0, 0, 0);
            cmd_push(CMD_BIND_VAO, vao_inst, 0, 0, 0, 0, 0, 0);
            cmd_push(CMD_USE_PROGRAM, prog_inst, 0, 0, 0, 0, 0, 0);
            cmd_push(CMD_DRAW_ARRAYS_INSTANCED, GL_TRIANGLES, 0, 6, inst_count, 0, 0, 0);
        }
    }

    // 2. Draw Scene
    scene.render(rc, t);

    // 3. Draw UI
    int ctx = SHINE_ENGINE.getCtx();
    shine::ui::UIManager::instance().onRender(ctx);
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
    buf = shine::wasm::SArray<float>(total);
    tri_count = count;
}

void DemoGame::ensure_instanced(int count) {
    if (count < 1) count = 1;
    if (count > 20000) count = 20000;
    inst_count = count;
    inst = shine::wasm::SArray<float>((unsigned int)count * 6u);
}

void DemoGame::update_vertices(float t) {
    // Local copy to prevent aliasing/reloading
    const int count = tri_count;
    if (count <= 0 || !buf.data()) return;

 
    const float aspect = SHINE_ENGINE.aspect; 
    
    float sx = 1.0f;
    if (shine::math::f_abs(aspect) > 0.00001f) sx = 1.0f / aspect;

    int grid = 1;
    while (grid * grid < count) grid++;

    float cell = 2.0f / (float)grid;
    float size = cell * 0.28f;
    const float inv_grid = 1.0f / (float)grid;
    const float b = 0.5f + 0.5f * shine::math::sin(t);
    const float t_base = t * 0.1f;

    float* outp = buf.data();
    int remaining = count;
    
    float base_pos_off = -1.0f + 0.5f * cell;
    
    // Hoist variables to avoid repeated stack allocation
    float cx, cy, r, g;
    float cx_size, cx_nsize, cy_size, cy_nsize;
    float sx_cx, sx_cx_size, sx_cx_nsize;

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
        const float cx_sin_offset = shine::math::sin(t_base + gy_f * 0.1f) * 0.05f;
        g = gy_f * inv_grid;

        // Strength reduction for inner loop
        float cx_linear = cx_linear_base;
        float cos_arg = cos_arg_base;
        float r_val = r_val_base;
        
        for (int gx = 0; gx < row_limit; ++gx) {
            cx = cx_linear + cx_sin_offset;
            // Cos depends on gx, but we use incremental arg
            cy = cy_base_row + shine::math::cos(cos_arg) * 0.05f;
            r = r_val;
            
            cx_size = cx + size;
            cx_nsize = cx - size;
            cy_size = cy + size;
            cy_nsize = cy - size;
            
            sx_cx = cx * sx;
            sx_cx_size = cx_size * sx;
            sx_cx_nsize = cx_nsize * sx;

            // Vertex 1
            *outp++ = sx_cx; *outp++ = cy_size; *outp++ = r; *outp++ = g; *outp++ = b;
            // Vertex 2
            *outp++ = sx_cx_nsize; *outp++ = cy_nsize; *outp++ = r; *outp++ = g; *outp++ = b;
            // Vertex 3
            *outp++ = sx_cx_size; *outp++ = cy_nsize; *outp++ = r; *outp++ = g; *outp++ = b;

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
    for (int i = 0; i < n; ++i) {
        int gx = i % grid;
        int gy = i / grid;
        float fi = (float)i;
        
        float cx = -1.0f + (gx + 0.5f) * cell;
        float cy = -1.0f + (gy + 0.5f) * cell;
        
        // Inline tri_wave/tri01 logic manually if needed, or trust compiler inliner
        // Simplified for size:
        float dx = shine::math::sin(t + fi * k0) * offset_scale; // replaced tri_wave with sin for size? no, keep math
        float dy = shine::math::sin(t + fi * k1) * offset_scale;
        float sc = scale_base + scale_var * (0.5f + 0.5f * shine::math::sin(t + fi * k2));

        float r = 0.5f + 0.5f * shine::math::sin(t + fi * k3);
        float g = 0.5f + 0.5f * shine::math::sin(t + fi * k4 + 2.0f);
        float b = 0.5f + 0.5f * shine::math::sin(t + fi * k5 + 4.0f);

        *outp++ = cx + dx;
        *outp++ = cy + dy;
        *outp++ = sc;
        *outp++ = r;
        *outp++ = g;
        *outp++ = b;
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
