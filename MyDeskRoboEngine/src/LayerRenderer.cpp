#include "LayerRenderer.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace nse {
namespace {

inline size_t alpha4_bytes(int16_t width, int16_t height) {
  return static_cast<size_t>(((width + 1) / 2) * height);
}

inline uint8_t alpha4_get(const uint8_t *buf, int16_t width, int16_t x, int16_t y) {
  const size_t idx = static_cast<size_t>(y) * static_cast<size_t>((width + 1) / 2) + static_cast<size_t>(x / 2);
  const uint8_t b = buf[idx];
  return (x & 1) ? (b & 0x0F) : ((b >> 4) & 0x0F);
}

inline void alpha4_set(uint8_t *buf, int16_t width, int16_t x, int16_t y, uint8_t v) {
  const size_t idx = static_cast<size_t>(y) * static_cast<size_t>((width + 1) / 2) + static_cast<size_t>(x / 2);
  v &= 0x0F;
  if (x & 1) {
    buf[idx] = static_cast<uint8_t>((buf[idx] & 0xF0) | v);
  } else {
    buf[idx] = static_cast<uint8_t>((buf[idx] & 0x0F) | (v << 4));
  }
}

}  // namespace

bool LayerRenderer::ensure_buffer(int16_t width, int16_t height) {
  if (!canvas_) return false;
  if (alpha_buf_ && width == width_ && height == height_) return true;

  if (alpha_buf_) {
    free(alpha_buf_);
    alpha_buf_ = nullptr;
  }

  const size_t bytes = alpha4_bytes(width, height);
  alpha_buf_ = static_cast<uint8_t *>(malloc(bytes));
  if (!alpha_buf_) return false;

  width_ = width;
  height_ = height;
  lv_canvas_set_buffer(canvas_, alpha_buf_, width_, height_, LV_IMG_CF_ALPHA_4BIT);
  lv_obj_set_size(canvas_, width_, height_);
  lv_obj_set_style_img_recolor_opa(canvas_, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_opa(canvas_, LV_OPA_TRANSP, 0);
  return true;
}

void LayerRenderer::clear_buffer() {
  if (!alpha_buf_ || width_ <= 0 || height_ <= 0) return;
  memset(alpha_buf_, 0, alpha4_bytes(width_, height_));
}

bool LayerRenderer::render_eye(const EyeSceneSpec &scene, const RuntimeState &state, bool right_eye) {
  const int16_t local_width = scene.geometry.virtual_width / 2;
  if (!ensure_buffer(local_width, kEyeViewportHeight)) {
    Serial.println("[RENDER] ensure_buffer failed");
    return false;
  }

  origin_x_ = right_eye ? local_width : 0;
  RuntimeState local_state = state;
  if (local_state.glitch_row_shift) {
    local_state.glitch_row_y -= kEyeViewportY;
  }
  for (uint8_t i = 0; i < local_state.glitch_scanline_count && i < 4; ++i) {
    local_state.glitch_scanline_y[i] -= kEyeViewportY;
  }

  if (lv_obj_get_parent(canvas_)) {
    lv_obj_set_style_bg_color(lv_obj_get_parent(canvas_), scene.background, 0);
    lv_obj_set_style_bg_opa(lv_obj_get_parent(canvas_), LV_OPA_COVER, 0);
  }
  lv_obj_set_style_bg_color(canvas_, scene.background, 0);
  lv_obj_set_style_bg_opa(canvas_, LV_OPA_COVER, 0);
  lv_obj_set_style_img_recolor(canvas_, state.eye_color, 0);
  lv_obj_set_style_opa(canvas_, state.output_opa ? state.output_opa : LV_OPA_COVER, 0);

  clear_buffer();
  draw_side(scene, local_state, right_eye);

  if (local_state.glitch_row_shift) {
    apply_row_shift(local_state.glitch_row_y, local_state.glitch_row_h, local_state.glitch_row_offset);
  }
  if (local_state.glitch_scanline_count > 0) {
    apply_scanlines(local_state);
  }

  lv_obj_invalidate(canvas_);
  return true;
}

void LayerRenderer::draw_side(const EyeSceneSpec &scene, const RuntimeState &state, bool right) {
  for (size_t i = 0; i < scene.op_count; ++i) {
    const RenderOp &op = scene.ops[i];
    if (!op.enabled) continue;
    if (op.side == Side::Left && right) continue;
    if (op.side == Side::Right && !right) continue;
    draw_op(scene, state, op, right);
  }
}

void LayerRenderer::draw_op(const EyeSceneSpec &scene, const RuntimeState &state, const RenderOp &op, bool right) {
  int16_t w = op.size.w;
  int16_t h = op.size.h;
  if (op.op == LayerOp::DrawGlow) {
    w = static_cast<int16_t>(w + state.pulse_shift);
    h = static_cast<int16_t>(h + (state.pulse_shift / 2));
  } else if (op.op == LayerOp::DrawMid) {
    w = static_cast<int16_t>(w + (state.pulse_shift / 2));
  }

  const bool is_wow = strcmp(scene.name, "eve_wow") == 0;
  if (is_wow && op.color.mode != ColorMode::SceneBackground) {
    w = static_cast<int16_t>(w + state.pulse_shift * 2);
    h = static_cast<int16_t>(h + state.pulse_shift * 2);
  }

  if (w < 4) w = 4;
  if (h < 4) h = 4;
  if (state.blink && op.op == LayerOp::DrawCore) {
    h = state.blink_height > 0 ? state.blink_height : 8;
  }

  const bool subtract = (op.op == LayerOp::DrawCut) || (op.color.mode == ColorMode::SceneBackground);
  const int16_t min_wh = w < h ? w : h;
  int16_t radius = op.primitive == Primitive::Ellipse ? static_cast<int16_t>(min_wh / 2) : op.radius;
  if (radius < 0) radius = 0;

  int16_t cy = side_y(scene, state, op);
  if (is_wow && op.color.mode != ColorMode::SceneBackground) {
    cy -= state.pulse_shift / 2;
  }

  blend_rounded_rect(side_x(scene, state, op, right), cy, w, h, radius, side_angle(op, right), op.opacity, subtract);
}

lv_color_t LayerRenderer::resolve_color(const EyeSceneSpec &scene, const RuntimeState &state, const ColorRef &color_ref) const {
  switch (color_ref.mode) {
    case ColorMode::RuntimeEye:
      return state.eye_color;
    case ColorMode::SceneBackground:
      return scene.background;
    case ColorMode::Custom:
    default:
      return color_ref.value;
  }
}

int16_t LayerRenderer::side_x(const EyeSceneSpec &scene, const RuntimeState &state, const RenderOp &op, bool right) const {
  const int16_t center_x = scene.geometry.virtual_width / 2;
  const int16_t eye_center = right ? center_x + scene.geometry.eye_gap : center_x - scene.geometry.eye_gap;
  const int16_t offset_x = (right && op.mirror_x_for_right) ? -op.offset.x : op.offset.x;
  const int16_t pupil_off = op.is_pupil ? state.pupil_x : 0;
  return eye_center + offset_x + state.drift_x + state.saccade_x + pupil_off - origin_x_;
}

int16_t LayerRenderer::side_y(const EyeSceneSpec &scene, const RuntimeState &state, const RenderOp &op) const {
  const int16_t pupil_off = op.is_pupil ? state.pupil_y : 0;
  return scene.geometry.base_y + op.offset.y + state.drift_y + state.saccade_y + pupil_off - kEyeViewportY;
}

int16_t LayerRenderer::side_angle(const RenderOp &op, bool right) const {
  if (!right || !op.mirror_angle_for_right) return op.angle_deg;
  return -op.angle_deg;
}

bool LayerRenderer::contains_rounded_rect(float local_x, float local_y, float half_w, float half_h, float radius) const {
  const float ax = fabsf(local_x);
  const float ay = fabsf(local_y);
  if (ax > half_w || ay > half_h) return false;

  const float inner_w = half_w - radius;
  const float inner_h = half_h - radius;
  if (ax <= inner_w || ay <= inner_h) return true;

  const float dx = ax - inner_w;
  const float dy = ay - inner_h;
  return (dx * dx + dy * dy) <= (radius * radius);
}

void LayerRenderer::set_mask_alpha(int16_t x, int16_t y, uint8_t src_alpha, bool subtract) {
  if (!alpha_buf_ || x < 0 || y < 0 || x >= width_ || y >= height_) return;
  const uint8_t dst4 = alpha4_get(alpha_buf_, width_, x, y);
  const uint8_t dst8 = static_cast<uint8_t>(dst4 * 17U);
  uint8_t out8 = 0;
  if (subtract) {
    out8 = static_cast<uint8_t>((static_cast<uint16_t>(dst8) * static_cast<uint16_t>(255 - src_alpha)) / 255U);
  } else {
    out8 = static_cast<uint8_t>(src_alpha + ((static_cast<uint16_t>(dst8) * static_cast<uint16_t>(255 - src_alpha)) / 255U));
  }
  const uint8_t out4 = static_cast<uint8_t>((out8 + 8U) / 17U);
  alpha4_set(alpha_buf_, width_, x, y, out4);
}

void LayerRenderer::blend_rounded_rect(int16_t cx, int16_t cy, int16_t w, int16_t h, int16_t radius, int16_t angle_deg, uint8_t opacity, bool subtract) {
  if (!alpha_buf_ || opacity == 0) return;

  const float rad = angle_deg * 0.01745329252f;
  const float sin_a = sinf(rad);
  const float cos_a = cosf(rad);
  const float half_w = w / 2.0f;
  const float half_h = h / 2.0f;
  const float min_half = half_w < half_h ? half_w : half_h;
  const float rr = radius < min_half ? static_cast<float>(radius) : min_half;
  const int16_t bound_x = static_cast<int16_t>(ceilf(fabsf(half_w * cos_a) + fabsf(half_h * sin_a))) + 2;
  const int16_t bound_y = static_cast<int16_t>(ceilf(fabsf(half_w * sin_a) + fabsf(half_h * cos_a))) + 2;

  for (int16_t y = cy - bound_y; y <= cy + bound_y; ++y) {
    for (int16_t x = cx - bound_x; x <= cx + bound_x; ++x) {
      const float dx = static_cast<float>(x) - static_cast<float>(cx);
      const float dy = static_cast<float>(y) - static_cast<float>(cy);
      const float local_x = dx * cos_a + dy * sin_a;
      const float local_y = -dx * sin_a + dy * cos_a;
      if (contains_rounded_rect(local_x, local_y, half_w, half_h, rr)) {
        set_mask_alpha(x, y, opacity, subtract);
      }
    }
  }
}

void LayerRenderer::apply_row_shift(int16_t start_y, int16_t height, int16_t offset) {
  if (!alpha_buf_ || height <= 0 || offset == 0) return;
  if (start_y < 0) start_y = 0;
  if (start_y >= height_) return;
  if (start_y + height > height_) height = height_ - start_y;
  uint8_t temp[180] = {0};
  if (width_ > 180) return;

  for (int16_t y = start_y; y < start_y + height; ++y) {
    for (int16_t x = 0; x < width_; ++x) {
      temp[x] = alpha4_get(alpha_buf_, width_, x, y);
      alpha4_set(alpha_buf_, width_, x, y, 0);
    }
    for (int16_t x = 0; x < width_; ++x) {
      const int16_t src_x = x - offset;
      if (src_x >= 0 && src_x < width_) {
        alpha4_set(alpha_buf_, width_, x, y, temp[src_x]);
      }
    }
  }
}

void LayerRenderer::apply_scanlines(const RuntimeState &state) {
  for (uint8_t i = 0; i < state.glitch_scanline_count && i < 4; ++i) {
    const int16_t y0 = state.glitch_scanline_y[i];
    const int16_t h = state.glitch_scanline_h[i] < 1 ? 1 : state.glitch_scanline_h[i];
    for (int16_t y = y0; y < y0 + h && y < height_; ++y) {
      if (y < 0) continue;
      for (int16_t x = 0; x < width_; ++x) {
        set_mask_alpha(x, y, 90, false);
      }
    }
  }
}

} // namespace nse
