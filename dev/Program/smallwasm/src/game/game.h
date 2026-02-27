#pragma once

#include "../engine/engine.h"

// Abstract base class for Game Logic
class Game {
public:
    virtual ~Game() = default;

    virtual void onInit() {}
    virtual void onResize(int w, int h) {}
    virtual void onUpdate(float t) {}
    virtual void onRender(float t) {}
    virtual void onPointer(float x_ndc, float y_ndc, int isDown) {}
};

// Factory function to be implemented by the specific game (e.g. demo_game.cpp)
extern Game* CreateGame();
