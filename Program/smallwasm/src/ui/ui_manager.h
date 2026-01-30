#pragma once

#include "ui.h"
#include "../Container/SVector.h"

namespace shine::ui {

class UIManager {
public:
    static UIManager& instance();
    void markDirty(Element* e);

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
    static constexpr int kBinX = 8;
    static constexpr int kBinY = 8;
    static constexpr int kBinCount = kBinX * kBinY;

    shine::wasm::SVector<Element*> m_elements;
    shine::wasm::SVector<Element*> m_visible;
    shine::wasm::SVector<Element*> m_bins[kBinCount];
    int m_binX = 1;
    int m_binY = 1;
    int m_viewW = 0;
    int m_viewH = 0;
    int m_pendingW = 0;
    int m_pendingH = 0;
    Element* m_capture = nullptr;
    bool m_visible_dirty = true;
    bool m_resize_dirty = false;
    shine::wasm::SVector<Element*> m_dirty;

    void rebuildVisible() noexcept;
    void rebuildDirty() noexcept;
    void applyResize() noexcept;
    void addToBins(Element* e, float viewW, float viewH, float cellW, float cellH);
    int binIndex(float x, float y, float cellW, float cellH) const;
};

} // 
