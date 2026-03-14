#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveSleepyOps[] = {
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {68, 113}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)110},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {62, 104}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "CutBottom", LayerOp::DrawCut, Primitive::RoundedRect, Side::Both, true, true, {0, 36}, {101, 47}, 3, 0, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
    {true, "CutTop", LayerOp::DrawCut, Primitive::RoundedRect, Side::Both, true, true, {0, -27}, {104, 70}, 3, 0, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
};

static const EyeSceneSpec kEveSleepyScene = {
    "eve_sleepy",
    {360, 360, 68, 180},
    lv_color_hex(0x06080d),
    kAnimSleepyStill,
    kEveSleepyOps,
    sizeof(kEveSleepyOps) / sizeof(kEveSleepyOps[0]),
};

} // namespace nse
