#include "ui_manager.h"

namespace shine {
namespace ui {

static UIManager s_uiMgr;

UIManager& UIManager::instance() {
    return s_uiMgr;
}

void UIManager::add(Element* e) {
    if (!e) return;
    m_elements.push_back(e);
}

void UIManager::remove(Element* e) {
    if (!e) return;
    for (unsigned int i = 0; i < m_elements.size(); ++i) {
        if (m_elements[i] == e) {
            // fast remove (swap with last) if order doesn't matter, 
            // but for UI z-order usually matters.
            // SVector doesn't have erase easily, so let's just null it or rebuild?
            // For now, let's just use swap-remove and assume z-order isn't critical or managed elsewhere.
            // Actually, let's keep it simple: swap with last and pop.
            // But wait, that changes order.
            // SVector erase is O(N).
            // Let's implement O(N) shift.
            for (unsigned int j = i; j < m_elements.size() - 1; ++j) {
                m_elements[j] = m_elements[j+1];
            }
            m_elements.pop_back();
            return;
        }
    }
}

void UIManager::clear() {
    m_elements.clear();
}

void UIManager::onResize(int w, int h) {
    for (unsigned int i = 0; i < m_elements.size(); ++i) {
        Element* e = m_elements[i];
        if (e) e->onResize(w, h);
    }
}

void UIManager::onRender(int ctxId) {
    for (unsigned int i = 0; i < m_elements.size(); ++i) {
        Element* e = m_elements[i];
        if (e && e->visible) {
            e->render(ctxId);
        }
    }
}

void UIManager::onPointer(float px, float py, int isDown) {
    // Iterate backwards for pointer (top-most first) usually?
    // But our render is forward (painters algo).
    // So top-most is last.
    // If we want to capture events, we should iterate backwards.
    // But our Element::pointer implementation is simple state update.
    // It doesn't consume events.
    for (unsigned int i = 0; i < m_elements.size(); ++i) {
        Element* e = m_elements[i];
        if (e && e->visible) {
            e->pointer(px, py, isDown);
        }
    }
}

} // namespace ui
} // namespace shine
