#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveDizzyOps[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {58, 58}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)36},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {50, 50}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "InnerCut", LayerOp::DrawCut, Primitive::RoundedRect, Side::Both, true, true, {10, 0}, {26, 30}, 6, 0, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
};

static const EyeSceneSpec kEveDizzyScene = {
    "eve_dizzy",
    {360, 360, 68, 180},
    lv_color_hex(0x06080d),
    kAnimDizzy,
    kEveDizzyOps,
    sizeof(kEveDizzyOps) / sizeof(kEveDizzyOps[0]),
};

} // namespace nse
