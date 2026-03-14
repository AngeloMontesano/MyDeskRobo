#pragma once

#include "SceneSpec.h"

namespace nse {

static constexpr int16_t kEyeViewportY = 60;
static constexpr int16_t kEyeViewportHeight = 240;

class LayerRenderer {
 public:
  explicit LayerRenderer(lv_obj_t *canvas) : canvas_(canvas) {}

  bool render_eye(const EyeSceneSpec &scene, const RuntimeState &state, bool right_eye);

 private:
  lv_obj_t *canvas_;
  uint8_t *alpha_buf_ = nullptr;
  int16_t width_ = 0;
  int16_t height_ = 0;
  int16_t origin_x_ = 0;

  bool ensure_buffer(int16_t width, int16_t height);
  void clear_buffer();
  void draw_side(const EyeSceneSpec &scene, const RuntimeState &state, bool right);
  void draw_op(const EyeSceneSpec &scene, const RuntimeState &state, const RenderOp &op, bool right);
  lv_color_t resolve_color(const EyeSceneSpec &scene, const RuntimeState &state, const ColorRef &color_ref) const;
  int16_t side_x(const EyeSceneSpec &scene, const RuntimeState &state, const RenderOp &op, bool right) const;
  int16_t side_y(const EyeSceneSpec &scene, const RuntimeState &state, const RenderOp &op) const;
  int16_t side_angle(const RenderOp &op, bool right) const;
  void blend_rounded_rect(int16_t cx, int16_t cy, int16_t w, int16_t h, int16_t radius, int16_t angle_deg, uint8_t opacity, bool subtract);
  bool contains_rounded_rect(float local_x, float local_y, float half_w, float half_h, float radius) const;
  void set_mask_alpha(int16_t x, int16_t y, uint8_t src_alpha, bool subtract);
  void apply_row_shift(int16_t start_y, int16_t height, int16_t offset);
  void apply_scanlines(const RuntimeState &state);
};

} // namespace nse
