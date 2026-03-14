#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveWowOps[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {76, 128}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)42},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {70, 122}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)104},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {66, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
};

static const EyeSceneSpec kEveWowScene = {
    "eve_wow",
    {360, 360, 70, 180},
    lv_color_hex(0x06080d),
    kAnimAlert,
    kEveWowOps,
    sizeof(kEveWowOps) / sizeof(kEveWowOps[0]),
};

} // namespace nse
