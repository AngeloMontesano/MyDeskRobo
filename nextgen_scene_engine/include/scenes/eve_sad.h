#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveSadOps[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 8}, {70, 108}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)34},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 8}, {64, 102}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)86},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 8}, {60, 96}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "Brow", LayerOp::DrawBrow, Primitive::RoundedRect, Side::Both, true, true, {6, -56}, {46, 7}, 3, 24, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)110},
};

static const EyeSceneSpec kEveSadScene = {
    "eve_sad",
    {360, 360, 66, 180},
    lv_color_hex(0x06080d),
    kAnimCalm,
    kEveSadOps,
    sizeof(kEveSadOps) / sizeof(kEveSadOps[0]),
};

} // namespace nse
