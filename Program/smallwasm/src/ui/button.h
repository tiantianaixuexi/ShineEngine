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
        const int result = TMINST.request_url(url, shine::wasm::raw_strlen(url), &style->bg_texId, nullptr, nullptr);
        LOG("resulet:", style->bg_texId);
    }

    inline void setBgDataUrl(const char* dataUrl) {
        TMINST.request_dataurl(dataUrl, shine::wasm::raw_strlen(dataUrl), &style->bg_texId, nullptr, nullptr);
    }

    inline void setBgBase64(const char* mime, const char* b64) {
        TMINST.request_base64(mime,
            shine::wasm::raw_strlen(mime),
            b64, shine::wasm::raw_strlen(b64),
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
        const bool wasOver = isOver;
        const bool nowOver = hit(px, py);
        isOver = nowOver;

        if (nowOver && !wasOver && HoverEvent) {
            HoverEvent(this);
            needRenderColor = &style->bg_hot;
        } else if (!nowOver && wasOver && UnHoverEvent) {
            UnHoverEvent(this);
            needRenderColor = &style->bg_idle;
        }

        if (isDown) {
            if (!_wasDown) {
                clicked = 0;
                if (nowOver) isPressed = true;
            }
            _wasDown = 1;
            return;
        }

        if (_wasDown) {
            if (isPressed && nowOver) {
                needRenderColor = &style->bg_hot;
                clicked = 1;
                if (ClickEvent) ClickEvent(this);
            }
            isPressed = false;
        }
        _wasDown = 0;
    }

    void onResize(int view_w, int view_h) override {
        Element::onResize(view_w, view_h);
    }

    void render(int ctxId) override {
        
        if(!visible) return;

        const ButtonStyle* s = style;


        // Make hover feedback VERY obvious (even with bg texture):
        float border_px = s->border_px;
        float border_r = s->border_color.r;
        float border_g = s->border_color.g;
        float border_b = s->border_color.b;
        float border_a = s->border_color.a;
        float texTint_a = s->bg_tex_tint.a;

        // if (isOver && !isPressed) {
        //     border_px = border_px * 2.0f;
        //     border_r = 1.0f;
        //     border_g = 1.0f;
        //     border_b = 1.0f;
        //     border_a = 0.85f;
        //     texTint_a = texTint_a + 0.25f;
        //     if (texTint_a > 1.0f) texTint_a = 1.0f;
        // }


        
        // Rounded background (optionally textured) + border + shadow.
        graphics::Renderer2D::instance().drawRoundRect(
            x, y, w, h,
            s->radius_px,
            needRenderColor->r, needRenderColor->g, needRenderColor->b, needRenderColor->a,
            s->bg_texId,
            s->bg_tex_tint.r, s->bg_tex_tint.g, s->bg_tex_tint.b, texTint_a,
            border_px,
            border_r, border_g, border_b, border_a,
            s->shadow_offset_px_x, s->shadow_offset_px_y,
            s->shadow_blur_px, s->shadow_spread_px,
            s->shadow_color.r, s->shadow_color.g, s->shadow_color.b, s->shadow_color.a
        );


    }

};

}// namespace shine::ui
