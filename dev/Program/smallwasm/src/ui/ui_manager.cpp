#include "ui_manager.h"
#include "../util/math_def.h"

namespace shine::ui {

static UIManager s_uiMgr;



UIManager& UIManager::instance() { return s_uiMgr; }

void ui_mark_dirty(Element* e) {
    if (e) UIManager::instance().markDirty(e);
}

void UIManager::markDirty(Element* e) {
    if (!e || e->uiDirty) return;
    e->uiDirty = true;
    m_dirty.push_back(e);
}

void UIManager::add(Element* e) {
    if (!e) return;
    m_elements.push_back(e);
    m_visible_dirty = true;
    if (m_viewW > 0 && m_viewH > 0) {
        e->onResize(m_viewW, m_viewH);
    }
}

void UIManager::remove(Element* e) {
    if (!e) return;
    m_elements.erase_first_unordered(e);
    // Also remove from dirty list if present
    for (unsigned i = 0; i < m_dirty.size(); ) {
        if (m_dirty[i] == e) {
            m_dirty.erase_first_unordered(e);
            e->uiDirty = false;
        } else {
            ++i;
        }
    }
    m_visible_dirty = true;
}

void UIManager::clear() {
    m_elements.clear();
    m_visible.clear();
    m_dirty.clear();
    m_capture = nullptr;
    m_visible_dirty = true;
}

void UIManager::onResize(int w, int h) {
    if (w == m_viewW && h == m_viewH) return;
    m_viewW = w;
    m_viewH = h;
    m_pendingW = w;
    m_pendingH = h;
    m_resize_dirty = true;
    m_visible_dirty = true;
}

void UIManager::applyResize() noexcept {
    m_resize_dirty = false;
    if (m_pendingW == 0 || m_pendingH == 0) return;
    for (Element* e : m_elements) {
        if (e) e->onResize(m_pendingW, m_pendingH);
    }
}

void UIManager::processElement(Element* e, float viewW, float viewH) {
    if (!e || !e->visible) return;
    const float hx = e->w * 0.5f;
    const float hy = e->h * 0.5f;
    
    const float minX = e->x - hx;
    const float maxX = e->x + hx;
    const float minY = e->y - hy;
    const float maxY = e->y + hy;

    if (maxX < 0.0f || minX > viewW || maxY < 0.0f || minY > viewH) {
        return;
    }
    
    m_visible.push_back(e);
}

inline void UIManager::rebuild() {
    if (m_resize_dirty) applyResize();

    if (!m_visible_dirty && m_dirty.empty()) return;

    const float viewW = (float)m_viewW;
    const float viewH = (float)m_viewH;

    if (m_visible_dirty) {
        m_visible.clear();
        for (Element* e : m_elements) processElement(e, viewW, viewH);
        m_visible_dirty = false;
    } else {
        for (Element* e : m_dirty) {
            if (!e) continue;
            m_visible.erase_first_unordered(e);
            processElement(e, viewW, viewH);
        }
    }
    
    for (Element* e : m_dirty) {
        if (e) e->uiDirty = false;
    }
    m_dirty.clear();
}


void UIManager::onRender(int ctxId) {
    if (m_elements.empty()) return;
    rebuild(); // rebuild only if needed
    for (Element* e : m_visible) {
        if (e) e->render(ctxId);
    }
}

void UIManager::onPointer(float px, float py, int isDown) {
    if (m_elements.empty()) return;
    rebuild();

    const unsigned int visible_count = m_visible.size();
    if (isDown) {
        if (!m_capture) {
            for (int i = (int)visible_count - 1; i >= 0; --i) {
                Element* e = m_visible[i];
                if (e && e->hit(px, py)) {
                    m_capture = e;
                    break;
                }
            }
        }
        if (m_capture) {
            m_capture->pointer(px, py, isDown);
            return;
        }
    } else {
        if (m_capture) {
            m_capture->pointer(px, py, isDown);
            m_capture = nullptr;
        }
    }

    // Pass pointer up event to all visible elements
    for (unsigned i = 0; i < visible_count; ++i) {
        Element* e = m_visible[i];
        if (e) e->pointer(px, py, 0);
    }
}

} // namespace ui