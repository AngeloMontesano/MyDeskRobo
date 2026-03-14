#include "LayerRenderer.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace nse {

bool LayerRenderer::ensure_buffer(int16_t width, int16_t height) {
  if (!canvas_) return false;
  if (alpha_buf_ && width == width_ && height == height_) return true;

  if (alpha_buf_) {
    free(alpha_buf_);
    alpha_buf_ = nullptr;
  }

  const size_t bytes = static_cast<size_t>(width) * static_cast<size_t>(height);
  alpha_buf_ = static_cast<uint8_t *>(malloc(bytes));
  if (!alpha_buf_) return false;

  width_ = width;
  height_ = height;
  lv_canvas_set_buffer(canvas_, alpha_buf_, width_, height_, LV_IMG_CF_ALPHA_8BIT);
  lv_obj_set_size(canvas_, width_, height_);
  lv_obj_set_style_img_recolor_opa(canvas_, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_opa(canvas_, LV_OPA_TRANSP, 0);
  return true;
}

void LayerRenderer::clear_buffer() {
  if (!alpha_buf_ || width_ <= 0 || height_ <= 0) return;
  memset(alpha_buf_, 0, static_cast<size_t>(width_) * static_cast<size_t>(height_));
}

void LayerRenderer::render(const EyeSceneSpec &scene, const RuntimeState &state) {
  if (!ensure_buffer(scene.geometry.virtual_width, scene.geometry.virtual_height)) return;

  if (lv_obj_get_parent(canvas_)) {
    lv_obj_set_style_bg_color(lv_obj_get_parent(canvas_), scene.background, 0);
    lv_obj_set_style_bg_opa(lv_obj_get_parent(canvas_), LV_OPA_COVER, 0);
  }
  lv_obj_set_style_img_recolor(canvas_, state.eye_color, 0);
  lv_obj_set_style_opa(canvas_, state.output_opa ? state.output_opa : LV_OPA_COVER, 0);

  clear_buffer();
  draw_side(scene, state, false);
  draw_side(scene, state, true);

  if (state.glitch_row_shift) {
    apply_row_shift(state.glitch_row_y, state.glitch_row_h, state.glitch_row_offset);
  }
  if (state.glitch_scanline_count > 0) {
    apply_scanlines(state);
  }

  lv_obj_invalidate(canvas_);
}

void LayerRenderer::render_pair(const EyeSceneSpec &left_scene, const RuntimeState &left_state,
                                const EyeSceneSpec &right_scene, const RuntimeState &right_state) {
  if (!ensure_buffer(left_scene.geometry.virtual_width, left_scene.geometry.virtual_height)) return;

  if (lv_obj_get_parent(canvas_)) {
    lv_obj_set_style_bg_color(lv_obj_get_parent(canvas_), left_scene.background, 0);
    lv_obj_set_style_bg_opa(lv_obj_get_parent(canvas_), LV_OPA_COVER, 0);
  }
  lv_obj_set_style_img_recolor(canvas_, left_state.eye_color, 0);
  lv_obj_set_style_opa(canvas_, left_state.output_opa ? left_state.output_opa : LV_OPA_COVER, 0);

  clear_buffer();
  draw_side(left_scene, left_state, false);
  draw_side(right_scene, right_state, true);

  if (left_state.glitch_row_shift || right_state.glitch_row_shift) {
    const RuntimeState &src = left_state.glitch_row_shift ? left_state : right_state;
    apply_row_shift(src.glitch_row_y, src.glitch_row_h, src.glitch_row_offset);
  }
  if (left_state.glitch_scanline_count > 0) {
    apply_scanlines(left_state);
  } else if (right_state.glitch_scanline_count > 0) {
    apply_scanlines(right_state);
  }

  lv_obj_invalidate(canvas_);
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
  int16_t radius = op.primitive == Primitive::Ellipse ? (int16_t)(min_wh / 2) : op.radius;
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
  return eye_center + offset_x + state.drift_x + state.saccade_x;
}

int16_t LayerRenderer::side_y(const EyeSceneSpec &scene, const RuntimeState &state, const RenderOp &op) const {
  return scene.geometry.base_y + op.offset.y + state.drift_y + state.saccade_y;
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
  uint8_t &dst = alpha_buf_[y * width_ + x];
  if (subtract) {
    dst = static_cast<uint8_t>((static_cast<uint16_t>(dst) * static_cast<uint16_t>(255 - src_alpha)) / 255U);
  } else {
    dst = static_cast<uint8_t>(src_alpha + ((static_cast<uint16_t>(dst) * static_cast<uint16_t>(255 - src_alpha)) / 255U));
  }
}

void LayerRenderer::blend_rounded_rect(int16_t cx, int16_t cy, int16_t w, int16_t h, int16_t radius, int16_t angle_deg, uint8_t opacity, bool subtract) {
  if (!alpha_buf_ || opacity == 0) return;

  const float rad = angle_deg * 0.01745329252f;
  const float sin_a = sinf(rad);
  const float cos_a = cosf(rad);
  const float half_w = w / 2.0f;
  const float half_h = h / 2.0f;
  const float min_half = half_w < half_h ? half_w : half_h;
  const float rr = radius < min_half ? (float)radius : min_half;
  const int16_t bound_x = (int16_t)ceilf(fabsf(half_w * cos_a) + fabsf(half_h * sin_a)) + 2;
  const int16_t bound_y = (int16_t)ceilf(fabsf(half_w * sin_a) + fabsf(half_h * cos_a)) + 2;

  for (int16_t y = cy - bound_y; y <= cy + bound_y; ++y) {
    for (int16_t x = cx - bound_x; x <= cx + bound_x; ++x) {
      const float dx = (float)x - (float)cx;
      const float dy = (float)y - (float)cy;
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
  uint8_t temp[360];
  if (width_ > 360) return;

  for (int16_t y = start_y; y < start_y + height; ++y) {
    uint8_t *row = alpha_buf_ + (y * width_);
    memcpy(temp, row, width_);
    memset(row, 0, width_);
    for (int16_t x = 0; x < width_; ++x) {
      const int16_t src_x = x - offset;
      if (src_x >= 0 && src_x < width_) row[x] = temp[src_x];
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
