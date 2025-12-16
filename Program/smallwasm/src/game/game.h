#pragma once

#include "../engine/engine.h"

// Abstract base class for Game Logic
class Game {
public:
    virtual ~Game() = default;

    virtual void onInit(shine::engine::Engine& app) {}
    virtual void onResize(shine::engine::Engine& app, int w, int h) {}
    virtual void onUpdate(shine::engine::Engine& app, float t) {}
    virtual void onRender(shine::engine::Engine& app, float t) {}
    virtual void onPointer(shine::engine::Engine& app, float x_ndc, float y_ndc, int isDown) {}
};

// Factory function to be implemented by the specific game (e.g. demo_game.cpp)
extern Game* CreateGame();
