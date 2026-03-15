#pragma once

#include "SceneCommon.h"

namespace nse {

// Flat upper cut (angle 0) → no angry tilt, just a narrowed intense stare.
// Pupil layer (DrawCut ellipse) creates dark centre on glowing iris → piercing focus effect.
static const RenderOp kEveFocusedOps[] = {
    {true, "Glow",  LayerOp::DrawGlow, Primitive::Ellipse,     Side::Both, true, true, {0, 0}, {74, 114}, 999, 0, {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true}, (lv_opa_t)42},
    {true, "Mid",   LayerOp::DrawMid,  Primitive::Ellipse,     Side::Both, true, true, {0, 0}, {68, 108}, 999, 0, {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true}, (lv_opa_t)110},
    {true, "Core",  LayerOp::DrawCore, Primitive::Ellipse,     Side::Both, true, true, {0, 0}, {62, 104}, 999, 0, {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "Cut",   LayerOp::DrawCut,  Primitive::RoundedRect, Side::Both, true, true, {0, -40}, {90, 26}, 3, 0, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
    {true, "Pupil", LayerOp::DrawCut,  Primitive::Ellipse,     Side::Both, true, true, {0,   4}, {28, 34}, 14, 0, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, (lv_opa_t)120, true},
};

static const EyeSceneSpec kEveFocusedScene = {
    "eve_focused",
    {360, 360, 68, 180},
    lv_color_hex(0x06080d),
    kAnimFocused,
    kEveFocusedOps,
    sizeof(kEveFocusedOps) / sizeof(kEveFocusedOps[0]),
};

} // namespace nse
