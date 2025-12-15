#pragma once

namespace shine { namespace ui {

struct Color4 {
  float r = 1.0f;
  float g = 1.0f;
  float b = 1.0f;
  float a = 1.0f;
};

struct ButtonStyle {
  Color4 bg_idle    { 1.0f, 1.0f, 1.0f, 1.0f };
  Color4 bg_hot		{ 0.8f, 0.8f, 0.8f, 1.0f };
  Color4 bg_active  { 0.9f, 0.9f, 0.9f, 1.0f };

  float radius_px = 10.0f;

  // Border (inside border).
  float border_px = 1.0f;
  Color4 border_color{1.0f, 1.0f, 1.0f, 0.15f};

  // Shadow (drop shadow behind).
  float shadow_offset_px_x = 0.0f;
  float shadow_offset_px_y = -2.0f;
  float shadow_blur_px = 8.0f;
  float shadow_spread_px = 0.0f;
  Color4 shadow_color{0.0f, 0.0f, 0.0f, 0.35f};

  // Optional background texture (JS texture handle). 0 = none.
  int bg_texId = 0;
  // Background texture tint (multiplied with sampled texture).
  Color4 bg_tex_tint{1.0f, 1.0f, 1.0f, 1.0f};
};

static ButtonStyle* DefaultButtonStyle() noexcept {
  ButtonStyle* style = new ButtonStyle();
  style->bg_idle   = {1.0f, 1.0f, 1.0f, 1.0f};
  style->bg_hot    = {0.8f, 0.8f, 0.8f, 1.0f};
  style->bg_active = {0.9f, 0.9f, 0.9f, 1.0f};
  style->radius_px = 0.0f;
  style->border_px = 0.0f;
  style->border_color = {0.0f, 0.0f, 0.0f, 0.0f};
  style->shadow_offset_px_x = 0.0f;
  style->shadow_offset_px_y = 0.0f;
  style->shadow_blur_px = 0.0f;
  style->shadow_spread_px = 0.0f;
  style->shadow_color = {0.0f, 0.0f, 0.0f, 0.0f};
  //style->bg_texId = bgTex;
  style->bg_tex_tint = {1.0f, 1.0f, 1.0f, 1.0f};
  return style;
};

} } // namespace shine::ui

