#pragma once

#include "../game/game.h"
#include "../game/scene.h"
#include "../ui/button.h"
#include "../ui/image.h"
#include "../engine/engine.h"
#include "../Container/SArray.h"

// Demo Game Implementation
// Moved out of gl_ctx.cpp

class DemoGame : public Game {
public:
    DemoGame() = default;
    virtual ~DemoGame() = default;

    void onInit() override;
    void onResize(int w, int h) override;
    void onUpdate(float t) override;
    void onRender(float t) override;
    void onPointer(float x_ndc, float y_ndc, int isDown) override;

    // Demo specific data
    shine::game::Scene scene;
    shine::game::RenderContext rc;

    shine::game::Node* player = nullptr;
    shine::game::Node* weapon = nullptr;

    // Demo render helpers
    void update_vertices(float t);
    void update_instances(float t);
    void ensure_buffer(int count);
    void ensure_instanced(int count);

    // Render mode: 0=Tri, 1=Instanced
    int render_mode = 0;

    // Tri demo
    int tri_count = 0;
    shine::wasm::SArray<float> buf;

    // Instanced demo
    int inst_count = 0;
    shine::wasm::SArray<float> inst;

    // Shader vars for raw demo (not part of Renderer2D yet)
    int prog = 0;
    int vbo = 0;
    int vao_basic = 0;

    int prog_inst = 0;
    int vbo_inst_base = 0;
    int vbo_inst_data = 0;
    int vao_inst = 0;

    // UI elements (owned by game in this demo)
    shine::ui::Button* btn = nullptr;
    shine::ui::Button* btn_mode = nullptr;
    shine::ui::Image* img = nullptr; // Changed to pointer for consistency

    // Texture loader (async) - Moved to Graphics/TextureManager
    // shine::wasm::TexLoader tex; 
};


// Global accessor for demo game instance (for callbacks)
extern DemoGame* g_demo_game;
