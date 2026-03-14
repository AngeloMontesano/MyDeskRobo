#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveSleepyOps[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 34}, {62, 20}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)30},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 34}, {58, 18}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)80},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 34}, {56, 16}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
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
