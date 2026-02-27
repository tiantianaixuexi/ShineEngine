#pragma once


#include "../util/timer.h"

// Forward declaration
class Game;

namespace shine::engine {


class Engine {
public:
    static Engine& instance() noexcept;

    // Lifecycle
    void init(int triCount);
    void onResize(int w, int h);
    void frame(float t);
    void pointer(float x, float y, int isDown);

    // Accessors
    int getWidth() const  noexcept { return m_width; }
    int getHeight() const noexcept{ return m_height; }
    int getFrameNo() const noexcept{ return m_frameNo; }
    bool isInited() const noexcept { return m_inited; }
    int getCtx() const  noexcept{ return m_ctx; }

    
    void getWidthHeight(int& out_w, int& out_h) const noexcept {
        out_w = m_width;
        out_h = m_height;
    }

    void getHalf(float& out_hw, float& out_hh) const noexcept {
        out_hw = half_w;
        out_hh = half_h;
    }

    // TODO: Move these to Renderer/Input systems later
    void setGame(Game* game) { m_game = game; }
    Game* getGame() const { return m_game; }

    // Temporary public access to globals until fully refactored
    // (We are moving step-by-step)
    bool m_inited = false;
    int m_ctx = 0;
    int m_width = 0;
    int m_height = 0;
    float half_w = 0.f;
    float half_h =0.f;
    float aspect = 0.0f;
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

//inline shine::engine::Engine sss;
// Global macro for easy access during refactoring
#define SHINE_ENGINE (shine::engine::Engine::instance())