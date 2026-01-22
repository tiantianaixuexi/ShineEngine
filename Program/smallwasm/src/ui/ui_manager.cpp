#include "ui_manager.h"

namespace shine::ui {

static UIManager s_uiMgr;

UIManager& UIManager::instance() {
    return s_uiMgr;
}

void UIManager::add(Element* e) {
    if (!e) return;
    m_elements.push_back(e);
    if (m_viewW > 0 && m_viewH > 0) {
        e->onResize(m_viewW, m_viewH);
    }
}

void UIManager::remove(Element* e) {
    if (!e) return;
    for (unsigned int i = 0; i < m_elements.size(); ++i) {
        if (m_elements[i] == e) {
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
    if (w == m_viewW && h == m_viewH) return;
    m_viewW = w;
    m_viewH = h;
    const unsigned int count = m_elements.size();
    for (unsigned int i = 0; i < count; ++i) {
        Element* e = m_elements[i];
        if (e) e->onResize(w, h);
    }
}

void UIManager::onRender(int ctxId) {
    const unsigned int count = m_elements.size();
    if (count == 0) return;
    for (unsigned int i = 0; i < count; ++i) {
        Element* e = m_elements[i];
        if (e && e->visible) {
            e->render(ctxId);
        }
    }
}

void UIManager::onPointer(float px, float py, int isDown) {
    if (m_elements.empty()) return;
    Element** begin = m_elements.data();
    Element** end = begin + m_elements.size();
    while (begin < end) {
        Element* e = *begin++;
        if (e && e->visible) {
            e->pointer(px, py, isDown);
        }
    }
}

} // namespace ui
