#pragma once

#include "ui.h"
#include "style.h"


namespace shine { namespace ui {

class Button : public Element {

public:
  // Optional shared style pointer (for reuse across many buttons).
    ButtonStyle* style = nullptr;

  // Callback binding (WASM-friendly, no STL).
    using OnClickFn = void(*)(Button* self, void* user);
    OnClickFn onClickFn = nullptr;
    void* onClickUser = nullptr;

  // state
    int clicked = 0; // set to 1 on mouse up inside


    inline void setStyle(ButtonStyle* s) { style = s; }

    inline void bindOnClick(OnClickFn fn, void* user = nullptr) {
        onClickFn = fn;
        onClickUser = user;
    }

    inline void setBgUrl(const char* url) {
        if (!style) return;
        ui_tex_request_url(url, shine::wasm::raw_strlen(url), &style->bg_texId, nullptr, nullptr);
    }

    inline void setBgDataUrl(const char* dataUrl) {
        if (!style) return;
        ui_tex_request_dataurl(dataUrl, shine::wasm::raw_strlen(dataUrl), &style->bg_texId, nullptr, nullptr);
    }

    inline void setBgBase64(const char* mime, const char* b64) {
        if (!style) return;
        ui_tex_request_base64(
            mime, 
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

        style = new ButtonStyle();
        style->bg_idle   = {0.18f, 0.18f, 0.20f, 1.0f};
        style->bg_hot    = {0.24f, 0.24f, 0.28f, 1.0f};
        style->bg_active = {0.14f, 0.34f, 0.82f, 1.0f};
        style->radius_px = 14.0f;
        style->border_px = 1.5f;
        style->border_color = {1.0f, 1.0f, 1.0f, 0.12f};
        style->shadow_offset_px_x = 0.0f;
        style->shadow_offset_px_y = -3.0f;
        style->shadow_blur_px = 12.0f;
        style->shadow_spread_px = 0.5f;
        style->shadow_color = {0.0f, 0.0f, 0.0f, 0.45f};
        //style->bg_texId = bgTex;
        style->bg_tex_tint = {1.0f, 1.0f, 1.0f, 0.35f};
    }

    void pointer(float px, float py, int isDown) override {
        const int hitNow = hit(px, py);
        hot = hitNow;

        if (isDown) {
            if (hitNow)
                active = 1;
            return;
        }

        // mouse up
        if (active && hitNow) {
            clicked = 1;
            if (onClickFn) onClickFn(this, onClickUser);
        }
        active = 0;
    }

    void onResize(int view_w, int view_h) override {
        Element::onResize(view_w, view_h);
    }

    void render(int ctxId) override {
        ButtonStyle local{};
        const ButtonStyle* s = style ? style : &local;

        const Color4* bg = &s->bg_idle;
        if (active) bg = &s->bg_active;
        else if (hot) bg = &s->bg_hot;

        // Make hover feedback VERY obvious (even with bg texture):
        float border_px = s->border_px;
        float border_r = s->border_color.r;
        float border_g = s->border_color.g;
        float border_b = s->border_color.b;
        float border_a = s->border_color.a;
        float texTint_a = s->bg_tex_tint.a;

        if (hot && !active) {
            border_px = border_px * 2.0f;
            border_r = 1.0f;
            border_g = 1.0f;
            border_b = 1.0f;
            border_a = 0.85f;
            texTint_a = texTint_a + 0.25f;
            if (texTint_a > 1.0f) texTint_a = 1.0f;
        }

        // Rounded background (optionally textured) + border + shadow.
        ui_draw_round_rect(
            ctxId, x, y, w, h,
            s->radius_px,
            bg->r, bg->g, bg->b, bg->a,
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

} } // namespace shine::ui
