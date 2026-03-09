#include "engine.h"
#include "../graphics/gl_api.h"
#include "../graphics/wasm_command_buffer.h"
#include "../graphics/renderer_2d.h"
#include "../ui/ui_manager.h"
#include "../util/wasm_compat.h"
#include "../game/game.h"
#include "../logfmt.h"

namespace shine::engine {

using namespace shine::graphics;

static SHINE_CONSTINIT const char kCanvasId[] = "c";

// Global static instance (safe in .bss)
static Engine s_engine_instance;

Engine& Engine::instance() noexcept {
    return s_engine_instance;
}

void Engine::init(int triCount) {

    if (m_ctx == 0) {
        m_ctx = js_create_context(ptr_i32(kCanvasId), sizeof(kCanvasId) - 1);
        LOG("ctxId", m_ctx);
    }
    m_inited = (m_ctx != 0);
    if (!m_inited) return;

    // Init systems
    graphics::Renderer2D::instance().init(m_ctx);

    // Create Game (Hardcoded DemoGame for now)
    if (!m_game) {
        // Use Factory
        m_game = CreateGame();
        
        m_game->onInit();
    }
}

void Engine::onResize(int w, int h) {

    m_width = w;
    m_height = h;
    half_w = 0.5f * m_width;
    half_h = 0.5f * m_height;
    aspect = (float)w / (float)h;

    graphics::Renderer2D::instance().m_viewW = w;
    graphics::Renderer2D::instance().m_viewH = h;

    shine::ui::UIManager::instance().onResize(w, h);

    if (m_game) {
        m_game->onResize(w, h);
    }

    
}


void Engine::frame(float t) {
    if (!m_inited) return;

    LOG("frame", t);

    m_timers.tick(t);

    // Reset command buffer
    graphics::cmd_reset();
    graphics::Renderer2D::instance().begin();


    
    CommandBuffer::Pass pass = g_cmd_buffer.begin_pass();

    // Set Viewport
    pass.push5(CMD_VIEWPORT, 0, 0, m_width, m_height);

    // Default Clear
    pass.push5(CMD_CLEAR_COLOR, f2i(0.07f), f2i(0.07f), f2i(0.07f), f2i(1.0f));
    pass.push2(CMD_CLEAR, GL_COLOR_BUFFER_BIT);

    // Game Update & Render
    if (m_game) {
        m_game->onUpdate(t);
        m_game->onRender(t);
    }

    graphics::Renderer2D::instance().end();

    // Submit commands
    CommandBuffer& cb = CommandBuffer::instance();
    int submit_count = 0;
    const int* submit_data = cb.getSubmitData(submit_count);
    gl_submit(m_ctx, ptr_i32(submit_data), submit_count);
    
    m_frameNo++;
}

void Engine::pointer(float x, float y, int isDown) {
    if (m_game) {
        m_game->onPointer(x, y, isDown);
    }
}

} // namespace engine
