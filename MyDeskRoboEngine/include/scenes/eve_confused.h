#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveConfusedOps[] = {
    {true, "LeftGlow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Left, false, false, {0, 0}, {74, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)42},
    {true, "LeftMid", LayerOp::DrawMid, Primitive::Ellipse, Side::Left, false, false, {0, 0}, {68, 108}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)110},
    {true, "LeftCore", LayerOp::DrawCore, Primitive::Ellipse, Side::Left, false, false, {0, 0}, {62, 104}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "RightGlow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Right, false, false, {0, 6}, {74, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)42},
    {true, "RightMid", LayerOp::DrawMid, Primitive::Ellipse, Side::Right, false, false, {0, 6}, {68, 108}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)110},
    {true, "RightCore", LayerOp::DrawCore, Primitive::Ellipse, Side::Right, false, false, {0, 6}, {62, 104}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "RightTopCut", LayerOp::DrawCut, Primitive::RoundedRect, Side::Right, false, false, {0, -36}, {88, 24}, 4, -8, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
};

static const EyeSceneSpec kEveConfusedScene = {
    "eve_confused",
    {360, 360, 68, 180},
    lv_color_hex(0x06080d),
    kAnimFocused,
    kEveConfusedOps,
    sizeof(kEveConfusedOps) / sizeof(kEveConfusedOps[0]),
};

} // namespace nse

