#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveAngrySoftOps[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {68, 122}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)40},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {62, 118}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)104},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {58, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "Cut", LayerOp::DrawCut, Primitive::RoundedRect, Side::Both, true, true, {2, -42}, {94, 32}, 3, 12, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
    {true, "Brow", LayerOp::DrawBrow, Primitive::RoundedRect, Side::Both, true, true, {7, -66}, {62, 9}, 3, 26, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)96},
};

static const EyeSceneSpec kEveAngrySoftScene = {
    "eve_angry_soft",
    {360, 360, 60, 180},
    lv_color_hex(0x06080d),
    kAnimAlert,
    kEveAngrySoftOps,
    sizeof(kEveAngrySoftOps) / sizeof(kEveAngrySoftOps[0]),
};

} // namespace nse
