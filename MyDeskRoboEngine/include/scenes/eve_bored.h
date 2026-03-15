#pragma once

#include "SceneCommon.h"

namespace nse {

// Heavy flat upper cut + slight lower cut → half-closed, expressionless.
// Visually between IDLE and SLEEPY — bored, not tired.
static const RenderOp kEveBored0ps[] = {
    {true, "Glow",      LayerOp::DrawGlow, Primitive::Ellipse,    Side::Both, true, true, {0,  0}, {74, 114}, 999, 0, {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true}, (lv_opa_t)42},
    {true, "Mid",       LayerOp::DrawMid,  Primitive::Ellipse,    Side::Both, true, true, {0,  0}, {68, 108}, 999, 0, {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true}, (lv_opa_t)110},
    {true, "Core",      LayerOp::DrawCore, Primitive::Ellipse,    Side::Both, true, true, {0,  0}, {62, 104}, 999, 0, {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "CutTop",    LayerOp::DrawCut,  Primitive::RoundedRect, Side::Both, true, true, {0, -28}, {104, 56}, 3, 0, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
    {true, "CutBottom", LayerOp::DrawCut,  Primitive::RoundedRect, Side::Both, true, true, {0,  40}, {104, 30}, 3, 0, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
    // Pupil sits in the visible slit; rolls upward behind CutTop for eye-roll effect.
    {true, "Pupil",     LayerOp::DrawCut,  Primitive::Ellipse,     Side::Both, true, true, {0,  10}, { 26, 32}, 13, 0, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, (lv_opa_t)120, true},
};

static const EyeSceneSpec kEveBoredScene = {
    "eve_bored",
    {360, 360, 68, 180},
    lv_color_hex(0x06080d),
    kAnimCalm,
    kEveBored0ps,
    sizeof(kEveBored0ps) / sizeof(kEveBored0ps[0]),
};

} // namespace nse
