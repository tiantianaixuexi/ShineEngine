#pragma once

// Minimal UI system (wasm-friendly, no STL).
// Coordinate space: Pixels. Origin top-left. +Y down.

#include "../util/wasm_compat.h"
#include "../logfmt.h"
namespace shine { namespace ui {

class Element {
public:

  // ==========================================
  // [1. 布局配置] (设置这些值来控制 UI 位置)
  // ==========================================

  // 锚点范围（UE5/UMG CanvasSlot 的 Anchors 概念，归一化 0..1）
  // - (0,0) = 左上角；(1,1) = 右下角
  // - min==max 表示不拉伸：元素挂在某个点上
  // - min!=max 表示拉伸：元素在锚点框内按边距撑开
  float anchor_min_x = 0.0f;
  float anchor_min_y = 0.0f;
  float anchor_max_x = 0.0f;
  float anchor_max_y = 0.0f;

  // 对齐/枢轴（UE5/UMG 的 Alignment / Pivot 概念，归一化 0..1）
  // - (0,0) 表示元素的左上角对齐到锚点解算出来的位置
  // - (0.5,0.5) 表示元素中心对齐
  // - (1,1) 表示右下角对齐
  float align_x = 0.0f;
  float align_y = 0.0f;

  // Offsets（像素）
  // - 非拉伸（min==max）时：
  //   - offset_left_px / offset_top_px      表示位置（相对锚点点的像素偏移）
  //   - offset_right_px / offset_bottom_px  表示尺寸（宽/高，像素）
  // - 拉伸（min!=max）时：
  //   - offset_left_px / offset_top_px      表示左/上边距（px）
  //   - offset_right_px / offset_bottom_px  表示右/下边距（px）
  float offset_left_px = 0.0f;
  float offset_top_px = 0.0f;
  float offset_right_px = 100.0f;
  float offset_bottom_px = 50.0f;

  // 相对尺寸（屏幕百分比，0.0 表示不启用）
  // - 仅在“非拉伸（min==max）”时生效，用来覆盖 offset_right_px/offset_bottom_px 的像素尺寸
  // - 例如 size_rel_w=0.2 表示宽度 = view_w * 0.2
  float size_rel_w = 0.0f;
  float size_rel_h = 0.0f;


  // ==========================================
  // [2. 计算结果] (系统自动计算，请勿手动修改)
  // ==========================================

  // 最终的屏幕绝对坐标 (像素)
  float x = 0.0f; // 中心 X
  float y = 0.0f; // 中心 Y
  float w = 100.0f; // 宽度
  float h = 50.0f;  // 高度

  // 当前视口大小缓存
  int view_w = 1;
  int view_h = 1;


  // ==========================================
  // [3. 内部状态]
  // ==========================================
  union {
      struct {
          bool visible : 1;   // 是否可见
          bool isOver : 1;    // 鼠标是否悬停
          bool isPressed : 1; // 鼠标是否按下
          bool reserved : 5;  // 保留位
      };
      unsigned char flags;   // 用于对齐/批量操作
  };

  // 8bit 对齐 
  unsigned char  _pad0[7];


  virtual ~Element() = default;

  virtual void init()
  {
      flags = 0;
      visible = true;
  }

  virtual  bool hit(float px, float py) const {
    const float hx = w * 0.5f;
    const float hy = h * 0.5f;
    return (px >= (x - hx) && px <= (x + hx) && py >= (y - hy) && py <= (y + hy)) ? 1 : 0;
  }

  virtual void pointer(float px, float py, int isDown) {
    isOver = hit(px, py);
    if (!isDown) isPressed = 0;
  }

  // 当视口大小改变时调用
  virtual void onResize(int view_w, int view_h) {
    LOG2("Element::onResize", view_w, view_h);
    this->view_w = (view_w < 1) ? 1 : view_w;
    this->view_h = (view_h < 1) ? 1 : view_h;

    const float vw = (float)this->view_w;
    const float vh = (float)this->view_h;

    const float ax0 = anchor_min_x * vw;
    const float ay0 = anchor_min_y * vh;
    const float ax1 = anchor_max_x * vw;
    const float ay1 = anchor_max_y * vh;

    const float dx = anchor_max_x - anchor_min_x;
    const float dy = anchor_max_y - anchor_min_y;
    const float adx = (dx < 0.0f) ? -dx : dx;
    const float ady = (dy < 0.0f) ? -dy : dy;
    const bool stretch_x = (adx > 0.000001f);
    const bool stretch_y = (ady > 0.000001f);

    float top_left_x = 0.0f;
    float top_left_y = 0.0f;

    if (stretch_x) {
      top_left_x = ax0 + offset_left_px;
      const float br_x = ax1 - offset_right_px;
      w = br_x - top_left_x;
    } else {
      top_left_x = ax0 + offset_left_px;
      if (size_rel_w > 0.0f) w = vw * size_rel_w;
      else w = offset_right_px;
    }

    if (stretch_y) {
      top_left_y = ay0 + offset_top_px;
      const float br_y = ay1 - offset_bottom_px;
      h = br_y - top_left_y;
    } else {
      top_left_y = ay0 + offset_top_px;
      if (size_rel_h > 0.0f) h = vh * size_rel_h;
      else h = offset_bottom_px;
    }

    if (w < 0.0f) w = 0.0f;
    if (h < 0.0f) h = 0.0f;

    top_left_x -= align_x * w;
    top_left_y -= align_y * h;

    x = top_left_x + w * 0.5f;
    y = top_left_y + h * 0.5f;
  }

  inline void setAnchors(float min_x, float min_y, float max_x, float max_y) noexcept {
    anchor_min_x = min_x;
    anchor_min_y = min_y;
    anchor_max_x = max_x;
    anchor_max_y = max_y;
  }

  inline void setAnchorPoint(float x01, float y01) noexcept {
    setAnchors(x01, y01, x01, y01);
  }

  inline void setAlignment(float ax01, float ay01) noexcept {
    align_x = ax01;
    align_y = ay01;
  }

  inline void setOffsetsPx(float left, float top, float right, float bottom) noexcept {
    offset_left_px = left;
    offset_top_px = top;
    offset_right_px = right;
    offset_bottom_px = bottom;
  }

  inline void setLayoutPx(float anchor_x01, float anchor_y01,
                          float pos_px_x, float pos_px_y,
                          float size_px_w, float size_px_h) noexcept {
    setAnchorPoint(anchor_x01, anchor_y01);
    offset_left_px = pos_px_x;
    offset_top_px = pos_px_y;
    offset_right_px = size_px_w;
    offset_bottom_px = size_px_h;
    size_rel_w = 0.0f;
    size_rel_h = 0.0f;
    onResize(view_w, view_h);
  }

  inline void setLayoutRel(float anchor_x01, float anchor_y01,
                           float pos_px_x, float pos_px_y,
                           float rel_w, float rel_h) noexcept {
    setAnchorPoint(anchor_x01, anchor_y01);
    offset_left_px = pos_px_x;
    offset_top_px = pos_px_y;
    size_rel_w = rel_w;
    size_rel_h = rel_h;
    onResize(view_w, view_h);
  }

  inline void setPosPx(float pxX, float pxY) noexcept {
    offset_left_px = pxX;
    offset_top_px = pxY;
    onResize(view_w, view_h);
  }

  inline void setSizePx(float pxW, float pxH) noexcept {
    offset_right_px = pxW;
    offset_bottom_px = pxH;
    size_rel_w = 0.0f;
    size_rel_h = 0.0f;
    onResize(view_w, view_h);
  }

  virtual void render(int /*ctxId*/)
  {
	  
  }
};



} } // namespace shine::ui

