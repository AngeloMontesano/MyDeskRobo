#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveWinkOps[] = {
    {true, "LeftGlow", LayerOp::DrawGlow, Primitive::RoundedRect, Side::Left, false, false, {0, -2}, {76, 14}, 7, -4, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)36},
    {true, "LeftLid", LayerOp::DrawShape, Primitive::RoundedRect, Side::Left, false, false, {0, -2}, {70, 10}, 5, -4, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "RightGlow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Right, false, false, {0, 0}, {74, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)42},
    {true, "RightMid", LayerOp::DrawMid, Primitive::Ellipse, Side::Right, false, false, {0, 0}, {68, 108}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)110},
    {true, "RightCore", LayerOp::DrawCore, Primitive::Ellipse, Side::Right, false, false, {0, 0}, {62, 104}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "RightHappyCut", LayerOp::DrawCut, Primitive::RoundedRect, Side::Right, false, false, {0, 36}, {101, 47}, 3, 0, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
};

static const EyeSceneSpec kEveWinkScene = {
    "eve_wink",
    {360, 360, 68, 180},
    lv_color_hex(0x06080d),
    kAnimIdle,
    kEveWinkOps,
    sizeof(kEveWinkOps) / sizeof(kEveWinkOps[0]),
};

} // namespace nse
