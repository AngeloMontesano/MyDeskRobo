#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <stddef.h>
#include <stdint.h>

namespace nse {

enum class LayerOp : uint8_t {
  DrawGlow,
  DrawMid,
  DrawCore,
  DrawShape,
  DrawCut,
  DrawBrow,
};

enum class Primitive : uint8_t {
  Ellipse,
  RoundedRect,
};

enum class Side : uint8_t {
  Both,
  Left,
  Right,
};

enum class ColorMode : uint8_t {
  RuntimeEye,
  SceneBackground,
  Custom,
};

struct Vec2i {
  int16_t x;
  int16_t y;
};

struct Size2i {
  int16_t w;
  int16_t h;
};

struct ColorRef {
  ColorMode mode;
  lv_color_t value;
  bool runtime_linked;
};

struct RenderOp {
  bool enabled;
  const char *name;
  LayerOp op;
  Primitive primitive;
  Side side;
  bool mirror_x_for_right;
  bool mirror_angle_for_right;
  Vec2i offset;
  Size2i size;
  int16_t radius;
  int16_t angle_deg;
  ColorRef color;
  lv_opa_t opacity;
  // If true, state.pupil_x/y is added on top of the normal saccade offset.
  // Leave unset (false) for all non-pupil ops — trailing zero-init handles it.
  bool is_pupil;
};

struct SceneGeometry {
  int16_t virtual_width;
  int16_t virtual_height;
  int16_t eye_gap;
  int16_t base_y;
};

struct BlinkProfile {
  bool enabled;
  uint16_t interval_ms;
  uint16_t duration_ms;
  int16_t closed_height;
};

struct DriftProfile {
  bool enabled;
  int16_t drift_x_amplitude;
  int16_t drift_y_amplitude;
  int16_t saccade_x_amplitude;
  int16_t saccade_y_amplitude;
  uint16_t drift_x_period_ms;
  uint16_t drift_y_period_ms;
  uint16_t saccade_x_period_ms;
  uint16_t saccade_y_period_ms;
};

struct PulseProfile {
  bool enabled;
  int16_t amplitude;
  uint16_t period_ms;
};

struct AnimationProfile {
  BlinkProfile blink;
  DriftProfile drift;
  PulseProfile pulse;
};

struct EyeSceneSpec {
  const char *name;
  SceneGeometry geometry;
  lv_color_t background;
  AnimationProfile animation;
  const RenderOp *ops;
  size_t op_count;
};

struct RuntimeState {
  lv_color_t eye_color;
  int16_t drift_x;
  int16_t drift_y;
  int16_t saccade_x;
  int16_t saccade_y;
  int16_t pupil_x;   // extra offset applied only to ops with is_pupil=true
  int16_t pupil_y;
  int8_t pulse_shift;
  bool blink;
  int16_t blink_height;
  lv_opa_t output_opa;
  bool glitch_row_shift;
  int16_t glitch_row_y;
  int16_t glitch_row_h;
  int16_t glitch_row_offset;
  uint8_t glitch_scanline_count;
  int16_t glitch_scanline_y[4];
  int16_t glitch_scanline_h[4];
};

} // namespace nse
