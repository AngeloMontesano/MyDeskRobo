#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveAngrySoftOps[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {74, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)42},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {68, 108}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)110},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {62, 104}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "Cut", LayerOp::DrawCut, Primitive::RoundedRect, Side::Both, true, true, {0, -45}, {84, 29}, 3, 8, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
};

static const EyeSceneSpec kEveAngrySoftScene = {
    "eve_angry_soft",
    {360, 360, 68, 180},
    lv_color_hex(0x06080d),
    kAnimAlert,
    kEveAngrySoftOps,
    sizeof(kEveAngrySoftOps) / sizeof(kEveAngrySoftOps[0]),
};

} // namespace nse
