#include "ui_manager.h"

namespace shine::ui {

static UIManager s_uiMgr;

static inline int clampi(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

UIManager& UIManager::instance() {
    return s_uiMgr;
}

void ui_mark_dirty(Element* e) {
    if (!e) return;
    UIManager::instance().markDirty(e);
}

void UIManager::rebuildVisible() noexcept {
    if (m_resize_dirty) applyResize();
    m_visible.clear();
    if (m_visible.capacity() < m_elements.size()) {
        m_visible.reserve(m_elements.size());
    }
    for (int i = 0; i < kBinCount; ++i) {
        m_bins[i].clear();
    }
    const float viewW = (float)m_viewW;
    const float viewH = (float)m_viewH;
    const unsigned int count = m_elements.size();
    int bins = 1;
    if (count > 128u) bins = 8;
    else if (count > 32u) bins = 4;
    else if (count > 8u) bins = 2;
    m_binX = bins;
    m_binY = bins;
    const float cellW = (m_binX > 0) ? (viewW / (float)m_binX) : 0.0f;
    const float cellH = (m_binY > 0) ? (viewH / (float)m_binY) : 0.0f;
    for (unsigned int i = 0; i < count; ++i) {
        Element* e = m_elements[i];
        if (!e || !e->visible) continue;
        const float hx = e->w * 0.5f;
        const float hy = e->h * 0.5f;
        if (e->x + hx < 0.0f || e->x - hx > viewW || e->y + hy < 0.0f || e->y - hy > viewH) {
            continue;
        }
        m_visible.push_back(e);

        addToBins(e, viewW, viewH, cellW, cellH);
    }
    m_visible_dirty = false;
    // Clear dirty list flags since we rebuilt everything.
    for (unsigned int i = 0; i < m_dirty.size(); ++i) {
        if (m_dirty[i]) m_dirty[i]->uiDirty = false;
    }
    m_dirty.clear();
}

void UIManager::rebuildDirty() noexcept {
    if (m_visible_dirty) {
        rebuildVisible();
        return;
    }
    if (m_resize_dirty) {
        applyResize();
        if (m_visible_dirty) {
            rebuildVisible();
            return;
        }
    }
    if (m_dirty.empty()) return;
    const float viewW = (float)m_viewW;
    const float viewH = (float)m_viewH;
    const float cellW = (m_binX > 0) ? (viewW / (float)m_binX) : 0.0f;
    const float cellH = (m_binY > 0) ? (viewH / (float)m_binY) : 0.0f;
    for (unsigned int i = 0; i < m_dirty.size(); ++i) {
        Element* e = m_dirty[i];
        if (!e) continue;
        e->uiDirty = false;
        m_visible.erase_first_unordered(e);
        for (int b = 0; b < kBinCount; ++b) {
            m_bins[b].erase_first_unordered(e);
        }
        if (!e->visible) continue;
        const float hx = e->w * 0.5f;
        const float hy = e->h * 0.5f;
        if (e->x + hx < 0.0f || e->x - hx > viewW || e->y + hy < 0.0f || e->y - hy > viewH) {
            continue;
        }
        m_visible.push_back(e);
        addToBins(e, viewW, viewH, cellW, cellH);
    }
    m_dirty.clear();
}

void UIManager::markDirty(Element* e) {
    if (!e) return;
    if (!e->uiDirty) return;
    for (unsigned int i = 0; i < m_dirty.size(); ++i) {
        if (m_dirty[i] == e) return;
    }
    m_dirty.push_back(e);
}

void UIManager::applyResize() noexcept {
    m_resize_dirty = false;
    if (m_pendingW == 0 || m_pendingH == 0) return;
    const unsigned int count = m_elements.size();
    for (unsigned int i = 0; i < count; ++i) {
        Element* e = m_elements[i];
        if (e) e->onResize(m_pendingW, m_pendingH);
    }
    m_visible_dirty = true;
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
    m_visible_dirty = true;
}

void UIManager::clear() {
    m_elements.clear();
    m_visible.clear();
    m_dirty.clear();
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

void UIManager::onRender(int ctxId) {
    if (m_elements.empty()) return;
    if (m_visible_dirty || !m_dirty.empty()) rebuildDirty();
    const unsigned int count = m_visible.size();
    for (unsigned int i = 0; i < count; ++i) {
        Element* e = m_visible[i];
        if (e) e->render(ctxId);
    }
}

void UIManager::onPointer(float px, float py, int isDown) {
    if (m_elements.empty()) return;
    if (m_visible_dirty || !m_dirty.empty()) rebuildDirty();
    const unsigned int visible_count = m_visible.size();
    if (isDown) {
        if (!m_capture) {
            bool hitFound = false;
            if (m_viewW > 0 && m_viewH > 0) {
                const float cellW = (float)m_viewW / (float)m_binX;
                const float cellH = (float)m_viewH / (float)m_binY;
                const int idx = binIndex(px, py, cellW, cellH);
                if (idx >= 0) {
                    shine::wasm::SVector<Element*>& bin = m_bins[idx];
                    for (int i = (int)bin.size() - 1; i >= 0; --i) {
                        Element* e = bin[(unsigned int)i];
                        if (e && e->hit(px, py)) {
                            m_capture = e;
                            hitFound = true;
                            break;
                        }
                    }
                }
            }
            if (!hitFound) {
                for (int i = (int)visible_count - 1; i >= 0; --i) {
                    Element* e = m_visible[(unsigned int)i];
                    if (e && e->hit(px, py)) {
                        m_capture = e;
                        break;
                    }
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

    for (unsigned int i = 0; i < visible_count; ++i) {
        Element* e = m_visible[i];
        if (e) e->pointer(px, py, 0);
    }
}

void UIManager::addToBins(Element* e, float viewW, float viewH, float cellW, float cellH) {
    if (cellW <= 0.0f || cellH <= 0.0f) return;
    const float hx = e->w * 0.5f;
    const float hy = e->h * 0.5f;
    const float minX = e->x - hx;
    const float maxX = e->x + hx;
    const float minY = e->y - hy;
    const float maxY = e->y + hy;
    int bx0 = clampi((int)(minX / cellW), 0, m_binX - 1);
    int bx1 = clampi((int)(maxX / cellW), 0, m_binX - 1);
    int by0 = clampi((int)(minY / cellH), 0, m_binY - 1);
    int by1 = clampi((int)(maxY / cellH), 0, m_binY - 1);
    for (int by = by0; by <= by1; ++by) {
        for (int bx = bx0; bx <= bx1; ++bx) {
            const int idx = by * m_binX + bx;
            m_bins[idx].push_back(e);
        }
    }
}

int UIManager::binIndex(float x, float y, float cellW, float cellH) const {
    if (cellW <= 0.0f || cellH <= 0.0f) return -1;
    int bx = clampi((int)(x / cellW), 0, m_binX - 1);
    int by = clampi((int)(y / cellH), 0, m_binY - 1);
    return by * m_binX + bx;
}

} // namespace ui
