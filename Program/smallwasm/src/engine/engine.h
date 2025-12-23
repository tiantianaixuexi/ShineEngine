#pragma once


#include "../util/timer.h"

// Forward declaration
class Game;

namespace shine::engine {

// Engine: Singleton managing the core application state.
// Replaces the old DemoApp and global g_* variables.
class Engine {
public:
    static Engine& instance();

    // Lifecycle
    void init(int triCount);
    void onResize(int w, int h);
    void frame(float t);
    void pointer(float x, float y, int isDown);

    // Accessors
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getFrameNo() const { return m_frameNo; }
    bool isInited() const { return m_inited; }
    int getCtx() const { return m_ctx; }

    // TODO: Move these to Renderer/Input systems later
    void setGame(Game* game) { m_game = game; }
    Game* getGame() const { return m_game; }

    // Temporary public access to globals until fully refactored
    // (We are moving step-by-step)
    int m_ctx = 0;
    bool m_inited = false;
    int m_width = 0;
    int m_height = 0;
    int m_frameNo = 0;

    // Timer
    shine::util::TimerQueue m_timers;

    // Game Logic Hook
    Game* m_game = nullptr;

    // Make constructor public to fix "calling a private constructor" error
    // when using static instance inside instance() or file scope.
    Engine() = default;
    ~Engine() = default;

private:
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
};

} // namespace engine
// Global macro for easy access during refactoring
#define SHINE_ENGINE (shine::engine::Engine::instance())
