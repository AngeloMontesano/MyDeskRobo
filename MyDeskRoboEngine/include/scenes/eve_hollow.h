#pragma once

#include "SceneCommon.h"

namespace nse {

// HOLLOW: DrawCut ellipse removes the iris interior → void/dark core.
// Remaining Glow + Mid rings form a glowing ring around empty eyes.
static const RenderOp kEveSunglassesRoundOps[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {74, 114}, 999, 0, {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true}, (lv_opa_t)42},
    {true, "Mid",  LayerOp::DrawMid,  Primitive::Ellipse, Side::Both, true, true, {0, 0}, {68, 108}, 999, 0, {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true}, (lv_opa_t)110},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {62, 104}, 999, 0, {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    // Hollow cut: ~10 px glowing ring remains visible around the void.
    {true, "Void", LayerOp::DrawCut,  Primitive::Ellipse, Side::Both, true, true, {0, 0}, {54,  88},  27, 0, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
};

static const EyeSceneSpec kEveSunglassesRoundScene = {
    "eve_hollow",
    {360, 360, 68, 180},
    lv_color_hex(0x06080d),
    kAnimIdle,
    kEveSunglassesRoundOps,
    sizeof(kEveSunglassesRoundOps) / sizeof(kEveSunglassesRoundOps[0]),
};

} // namespace nse
