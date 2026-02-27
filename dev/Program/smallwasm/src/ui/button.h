#pragma once

#include "ui.h"
#include "style.h"
#include "../graphics/renderer_2d.h"
#include "../graphics/texture_manager.h"
#include "../logfmt.h"

namespace shine::ui {

class Button : public Element {

public:
  // 可选的共享样式指针 (可在多个按钮间复用)
    ButtonStyle* style = nullptr;

    Color4* needRenderColor = nullptr;
    

  // 回调绑定 (WASM友好, 不使用STL)
    using OnButtonEvent = void(*)(Button* self);


    OnButtonEvent ClickEvent   = nullptr;
    OnButtonEvent HoverEvent   = nullptr;
    OnButtonEvent UnHoverEvent = nullptr;


  // 状态
    int clicked = 0; // 在内部鼠标抬起时设为 1
    int _wasDown = 0;
    

    inline void setStyle(ButtonStyle* s) noexcept { style = s; }

    inline void bindOnClick(OnButtonEvent fn)  noexcept
	{
        ClickEvent = fn;
    }

    inline void bindHoverEvent(OnButtonEvent fn) noexcept
    {
        HoverEvent =  fn;
    }

    inline void bindUnHoverEvent(OnButtonEvent fn) noexcept
    {
        UnHoverEvent = fn;
    }
    

    inline void setBgUrl(const char* url) {
        const int len = raw_strlen(url);
        TMINST.request_url(url, len, &style->bg_texId, nullptr, nullptr);
    }

    inline void setBgDataUrl(const char* dataUrl) {
        const int len = raw_strlen(dataUrl);
        TMINST.request_dataurl(dataUrl, len, &style->bg_texId, nullptr, nullptr);
    }

    inline void setBgBase64(const char* mime, const char* b64) {
        const int mime_len = raw_strlen(mime);
        const int b64_len = raw_strlen(b64);
        TMINST.request_base64(mime,
            mime_len,
            b64, b64_len,
            &style->bg_texId, nullptr, nullptr);
    }

    static Button* create() {
        Button* btn = new Button();
        btn->init();
        return btn;
    }

    void init() override {

        Element::init();

        style = DefaultButtonStyle();
        needRenderColor = &style->bg_idle;
    }

    void pointer(float px, float py, int isDown) override {
        const bool nowOver = hit(px, py);
        
        // Hover State Logic
        if (nowOver != isOver) {
            isOver = nowOver;
            needRenderColor = nowOver ? &style->bg_hot : &style->bg_idle;
            if (nowOver) {
                if (HoverEvent) HoverEvent(this);
            } else {
                if (UnHoverEvent) UnHoverEvent(this);
            }
        }

        // Click Logic
        if (isDown) {
            if (!_wasDown && nowOver) isPressed = true;
            _wasDown = 1;
        } else {
            if (_wasDown) {
                if (isPressed && nowOver) {
                    // Clicked!
                    needRenderColor = &style->bg_hot;
                    clicked = 1;
                    if (ClickEvent) ClickEvent(this);
                }
                isPressed = false;
                _wasDown = 0;
            }
        }
    }

    void onResize(int view_w, int view_h) override {
        Element::onResize(view_w, view_h);
    }

    void render(int ctxId) override {
        
        if(!visible) return;

        const ButtonStyle* s = style;


        // Make hover feedback VERY obvious (even with bg texture):
        float border_px = s->border_px;


        
        // Rounded background (optionally textured) + border + shadow.
        const graphics::Renderer2D::Rect rect{x, y, w, h};
        const graphics::Renderer2D::Color4 fill{needRenderColor->r, needRenderColor->g, needRenderColor->b, needRenderColor->a};
        const graphics::Renderer2D::Color4 texTint{s->bg_tex_tint.r, s->bg_tex_tint.g, s->bg_tex_tint.b, s->bg_tex_tint.a};
        const graphics::Renderer2D::Color4 borderColor{s->border_color.r, s->border_color.g, s->border_color.b, s->border_color.a};
        const graphics::Renderer2D::Color4 shadowColor{s->shadow_color.r, s->shadow_color.g, s->shadow_color.b, s->shadow_color.a};

        graphics::Renderer2D::RoundRectStyle style;
        style.radius_px = s->radius_px;
        style.fill = fill;
        style.texId = s->bg_texId;
        style.texTint = texTint;
        style.border_px = border_px;
        style.borderColor = borderColor;
        style.shadow_off_x = s->shadow_offset_px_x;
        style.shadow_off_y = s->shadow_offset_px_y;
        style.shadow_blur = s->shadow_blur_px;
        style.shadow_spread = s->shadow_spread_px;
        style.shadowColor = shadowColor;

        graphics::Renderer2D::instance().drawRoundRect(rect, style);


    }

};

}// namespace shine::ui
