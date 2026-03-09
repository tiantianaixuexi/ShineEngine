#pragma once

#include "ui.h"
#include "../Container/SVector.h"

namespace shine::ui {

class UIManager {
public:
    static UIManager& instance();
    void markDirty(Element* e);

    void add(Element* e);
    void remove(Element* e);
    void clear();

    void onResize(int w, int h);
    void onRender(int ctxId);
    void onPointer(float px, float py, int isDown);

private:
    shine::wasm::SVector<Element*> m_elements;
    shine::wasm::SVector<Element*> m_visible;
    int m_viewW = 0;
    int m_viewH = 0;
    int m_pendingW = 0;
    int m_pendingH = 0;
    Element* m_capture = nullptr;
    bool m_visible_dirty = true;
    bool m_resize_dirty = false;
    shine::wasm::SVector<Element*> m_dirty;

    inline void rebuild();
    void applyResize() noexcept;
    void processElement(Element* e, float viewW, float viewH);
};

} // namespace ui