#include "LayerRenderer.h"

namespace nse {

void LayerRenderer::render(const EyeSceneSpec &scene, const RuntimeState &state) {
  if (!canvas_) return;
  lv_obj_set_style_bg_color(canvas_, scene.background, 0);
  lv_obj_set_style_bg_opa(canvas_, LV_OPA_COVER, 0);

  size_t slot_index = 0;
  draw_side(scene, state, false, slot_index);
  draw_side(scene, state, true, slot_index);
  hide_unused_objects(slot_index);
}

void LayerRenderer::draw_side(const EyeSceneSpec &scene, const RuntimeState &state, bool right, size_t &slot_index) {
  for (size_t i = 0; i < scene.op_count; ++i) {
    const RenderOp &op = scene.ops[i];
    if (!op.enabled) continue;
    if (op.side == Side::Left && right) continue;
    if (op.side == Side::Right && !right) continue;
    draw_op(scene, state, op, right, slot_index);
  }
}

void LayerRenderer::draw_op(const EyeSceneSpec &scene, const RuntimeState &state, const RenderOp &op, bool right, size_t &slot_index) {
  lv_obj_t *obj = ensure_layer_object(slot_index);
  if (!obj) return;
  ++slot_index;

  int16_t w = op.size.w;
  int16_t h = op.size.h;
  if (op.op == LayerOp::DrawGlow) {
    w = static_cast<int16_t>(w + state.pulse_shift);
    h = static_cast<int16_t>(h + (state.pulse_shift / 2));
  } else if (op.op == LayerOp::DrawMid) {
    w = static_cast<int16_t>(w + (state.pulse_shift / 2));
  }
  if (w < 4) w = 4;
  if (h < 4) h = 4;
  if (state.blink && op.op == LayerOp::DrawCore) {
    h = state.blink_height > 0 ? state.blink_height : 8;
  }

  lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_size(obj, w, h);
  lv_obj_set_pos(obj, side_x(scene, state, op, right) - w / 2, side_y(scene, state, op) - h / 2);
  lv_obj_set_style_bg_color(obj, resolve_color(scene, state, op.color), 0);
  lv_obj_set_style_bg_opa(obj, op.opacity, 0);
  apply_primitive(obj, op.primitive, op);

  const int16_t angle = side_angle(op, right);
  lv_obj_set_style_transform_angle(obj, angle != 0 ? angle * 10 : 0, 0);
}

lv_obj_t *LayerRenderer::ensure_layer_object(size_t slot_index) {
  if (slot_index >= kMaxLayerObjects || !canvas_) return nullptr;
  if (!layer_objects_[slot_index]) {
    layer_objects_[slot_index] = lv_obj_create(canvas_);
    lv_obj_remove_style_all(layer_objects_[slot_index]);
  }
  if (slot_index + 1 > active_object_count_) active_object_count_ = slot_index + 1;
  return layer_objects_[slot_index];
}

void LayerRenderer::hide_unused_objects(size_t used_count) {
  for (size_t i = used_count; i < active_object_count_; ++i) {
    if (layer_objects_[i]) {
      lv_obj_add_flag(layer_objects_[i], LV_OBJ_FLAG_HIDDEN);
    }
  }
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

void LayerRenderer::apply_primitive(lv_obj_t *obj, Primitive primitive, const RenderOp &op) const {
  switch (primitive) {
    case Primitive::Ellipse:
      lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
      break;
    case Primitive::RoundedRect:
    default:
      lv_obj_set_style_radius(obj, op.radius, 0);
      break;
  }
}

} // namespace nse
