#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveExcitedOps[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 2}, {72, 104}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)44},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 2}, {66, 96}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)108},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 2}, {62, 88}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "Cut", LayerOp::DrawCut, Primitive::RoundedRect, Side::Both, true, true, {0, 30}, {88, 18}, 8, 0, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
};

static const EyeSceneSpec kEveExcitedScene = {
    "eve_excited",
    {360, 360, 66, 180},
    lv_color_hex(0x06080d),
    kAnimExcited,
    kEveExcitedOps,
    sizeof(kEveExcitedOps) / sizeof(kEveExcitedOps[0]),
};

} // namespace nse
