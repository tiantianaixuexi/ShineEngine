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

static const char kCanvasId[] = "c";

// Global static instance (safe in .bss)
static Engine s_engine_instance;

Engine& Engine::instance() {
    return s_engine_instance;
}

void Engine::init(int triCount) {

    if (m_ctx == 0) {
        m_ctx = js_create_context(wasm::ptr_i32(kCanvasId), sizeof(kCanvasId) - 1);
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
        
        m_game->onInit(*this);
    }
}

void Engine::onResize(int w, int h) {
    LOG2("Engine::onResize", w, h);
    m_width = w;
    m_height = h;
    
    graphics::Renderer2D::instance().m_viewW = w;
    graphics::Renderer2D::instance().m_viewH = h;

    shine::ui::UIManager::instance().onResize(w, h);

    if (m_game) {
        m_game->onResize(*this, w, h);
    }

    
}

static inline int f2i(float f) { return *(int*)&f; }

void Engine::frame(float t) {
    if (!m_inited) return;


    m_timers.tick(t);

    // Reset command buffer
    graphics::cmd_reset();
    graphics::Renderer2D::instance().begin();


    
    // Set Viewport
    cmd_push(CMD_VIEWPORT, 0, 0, m_width, m_height, 0, 0, 0);

    // Default Clear
    cmd_push(CMD_CLEAR_COLOR, f2i(0.07f), f2i(0.07f), f2i(0.07f), f2i(1.0f), 0, 0, 0);
    cmd_push(CMD_CLEAR, GL_COLOR_BUFFER_BIT, 0, 0, 0, 0, 0, 0);

    // Game Update & Render
    if (m_game) {
        m_game->onUpdate(*this, t);
        m_game->onRender(*this, t);
    }

    graphics::Renderer2D::instance().end();

    // Submit commands
    CommandBuffer& cb = CommandBuffer::instance();
    gl_submit(m_ctx, shine::wasm::ptr_i32(cb.getData()), cb.getCount());
    
    m_frameNo++;
}

void Engine::pointer(float x, float y, int isDown) {
    if (m_game) {
        m_game->onPointer(*this, x, y, isDown);
    }
}

} // namespace engine
