#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveConfusedOps[] = {
    {true, "GlowL", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {68, 122}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)36},
    {true, "MidL", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {62, 118}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)92},
    {true, "CoreL", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {58, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "RightCut", LayerOp::DrawCut, Primitive::RoundedRect, Side::Both, true, true, {-8, -6}, {58, 58}, 8, 0, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
    {true, "Brow", LayerOp::DrawBrow, Primitive::RoundedRect, Side::Both, true, true, {4, -54}, {38, 7}, 3, 10, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)92},
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
