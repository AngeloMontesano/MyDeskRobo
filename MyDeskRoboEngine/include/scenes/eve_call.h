#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveCallOps[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {82, 82}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)38},
    {true, "Ring", LayerOp::DrawShape, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {74, 74}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "InnerCut", LayerOp::DrawCut, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {48, 48}, 999, 0, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
};

static const EyeSceneSpec kEveCallScene = {
    "eve_call",
    {360, 360, 68, 180},
    lv_color_hex(0x06080d),
    kAnimAlert,
    kEveCallOps,
    sizeof(kEveCallOps) / sizeof(kEveCallOps[0]),
};

} // namespace nse
