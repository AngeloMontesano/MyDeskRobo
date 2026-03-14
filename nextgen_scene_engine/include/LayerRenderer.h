#pragma once

#include "SceneSpec.h"

namespace nse {

class LayerRenderer {
 public:
  explicit LayerRenderer(lv_obj_t *canvas) : canvas_(canvas) {}

  void render(const EyeSceneSpec &scene, const RuntimeState &state);

 private:
  static constexpr size_t kMaxLayerObjects = 64;

  lv_obj_t *canvas_;
  lv_obj_t *layer_objects_[kMaxLayerObjects] = {};
  size_t active_object_count_ = 0;

  void draw_side(const EyeSceneSpec &scene, const RuntimeState &state, bool right, size_t &slot_index);
  void draw_op(const EyeSceneSpec &scene, const RuntimeState &state, const RenderOp &op, bool right, size_t &slot_index);
  lv_obj_t *ensure_layer_object(size_t slot_index);
  void hide_unused_objects(size_t used_count);
  lv_color_t resolve_color(const EyeSceneSpec &scene, const RuntimeState &state, const ColorRef &color_ref) const;
  int16_t side_x(const EyeSceneSpec &scene, const RuntimeState &state, const RenderOp &op, bool right) const;
  int16_t side_y(const EyeSceneSpec &scene, const RuntimeState &state, const RenderOp &op) const;
  int16_t side_angle(const RenderOp &op, bool right) const;
  void apply_primitive(lv_obj_t *obj, Primitive primitive, const RenderOp &op) const;
};

} // namespace nse
