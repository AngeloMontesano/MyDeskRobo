#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveBaseCOps[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {74, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)42},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {68, 108}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)110},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {62, 104}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
};

static const EyeSceneSpec kEveBaseCScene = {
    "eve_base_c_glow_mid",
    {360, 360, 68, 180},
    lv_color_hex(0x06080d),
    kAnimStill,
    kEveBaseCOps,
    sizeof(kEveBaseCOps) / sizeof(kEveBaseCOps[0]),
};

} // namespace nse
