#pragma once

#include "ui.h"
#include "../Container/SVector.h"

namespace shine::ui {

class UIManager {
public:
    static UIManager& instance();

    // Add element to the manager (does NOT take ownership, caller must delete)
    void add(Element* e);
    
    // Remove element (does NOT delete)
    void remove(Element* e);
    
    // Clear all elements (does NOT delete)
    void clear();

    // Core loops
    void onResize(int w, int h);
    void onRender(int ctxId);
    void onPointer(float px, float py, int isDown);

private:
    shine::wasm::SVector<Element*> m_elements;
};

} // 