#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveAngryHardOps[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {70, 126}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)52},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {64, 120}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)128},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {58, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "Cut", LayerOp::DrawCut, Primitive::RoundedRect, Side::Both, true, true, {5, -54}, {116, 46}, 3, 28, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
    {true, "Glow2", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {70, 120}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)52},
    {true, "Brow", LayerOp::DrawBrow, Primitive::RoundedRect, Side::Both, true, true, {12, -79}, {84, 12}, 3, 48, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)140},
};

static const EyeSceneSpec kEveAngryHardScene = {
    "eve_angry_hard",
    {360, 360, 56, 180},
    lv_color_hex(0x06080d),
    kAnimExcited,
    kEveAngryHardOps,
    sizeof(kEveAngryHardOps) / sizeof(kEveAngryHardOps[0]),
};

} // namespace nse
