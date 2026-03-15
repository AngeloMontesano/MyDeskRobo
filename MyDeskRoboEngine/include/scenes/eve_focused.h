#pragma once

#include "SceneCommon.h"

namespace nse {

// Symmetrical slight upper cut on both eyes, minimal tilt → intense, focused stare.
// Eyes are narrower than idle but not droopy — looks concentrated, not angry.
static const RenderOp kEveFocusedOps[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse,    Side::Both, true, true, {0, 0}, {74, 114}, 999, 0, {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true}, (lv_opa_t)42},
    {true, "Mid",  LayerOp::DrawMid,  Primitive::Ellipse,    Side::Both, true, true, {0, 0}, {68, 108}, 999, 0, {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true}, (lv_opa_t)110},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse,    Side::Both, true, true, {0, 0}, {62, 104}, 999, 0, {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "Cut",  LayerOp::DrawCut,  Primitive::RoundedRect, Side::Both, true, true, {0, -43}, {80, 20}, 3, 5, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
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
