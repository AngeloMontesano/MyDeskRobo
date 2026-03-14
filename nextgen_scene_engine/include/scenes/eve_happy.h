#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveHappyOps[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 12}, {68, 96}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)38},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 12}, {62, 90}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)96},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 12}, {58, 84}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "SmileCut", LayerOp::DrawCut, Primitive::RoundedRect, Side::Both, true, true, {0, 42}, {86, 34}, 8, 0, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
};

static const EyeSceneSpec kEveHappyScene = {
    "eve_happy",
    {360, 360, 66, 180},
    lv_color_hex(0x06080d),
    kAnimExcited,
    kEveHappyOps,
    sizeof(kEveHappyOps) / sizeof(kEveHappyOps[0]),
};

} // namespace nse
