#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveBaseBOps[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {72, 112}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)40},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {62, 104}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
};

static const EyeSceneSpec kEveBaseBScene = {
    "eve_base_b_glow",
    {360, 360, 68, 180},
    lv_color_hex(0x06080d),
    kAnimStill,
    kEveBaseBOps,
    sizeof(kEveBaseBOps) / sizeof(kEveBaseBOps[0]),
};

} // namespace nse
