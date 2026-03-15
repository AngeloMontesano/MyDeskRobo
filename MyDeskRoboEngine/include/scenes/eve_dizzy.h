#pragma once

#include "SceneCommon.h"

namespace nse {

// X-eyes: two crossing DrawCut bars at ±42° carve a dark X through each iris.
// Leaves four bright lobes (top/bottom/left/right) → classic cartoon KO/dizzy look.
// mirror_angle_for_right=true ensures right eye gets the same X (angles flip but cross stays).
static const RenderOp kEveDizzyOps[] = {
    {true, "Glow",  LayerOp::DrawGlow, Primitive::Ellipse,     Side::Both, true, true, {0, 0}, {74, 114}, 999,  0, {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true},  (lv_opa_t)38},
    {true, "Mid",   LayerOp::DrawMid,  Primitive::Ellipse,     Side::Both, true, true, {0, 0}, {68, 108}, 999,  0, {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true},  (lv_opa_t)100},
    {true, "Core",  LayerOp::DrawCore, Primitive::Ellipse,     Side::Both, true, true, {0, 0}, {62, 104}, 999,  0, {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true},  LV_OPA_COVER},
    {true, "CutX1", LayerOp::DrawCut,  Primitive::RoundedRect, Side::Both, true, true, {0, 0}, {120, 18},   9, 42, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
    {true, "CutX2", LayerOp::DrawCut,  Primitive::RoundedRect, Side::Both, true, true, {0, 0}, {120, 18},   9, -42, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
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
