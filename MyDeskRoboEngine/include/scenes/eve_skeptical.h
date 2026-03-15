#pragma once

#include "SceneCommon.h"

namespace nse {

// Left eye: fully open (idle-like).
// Right eye: squinted with an outward-tilted upper cut + matching brow
// → classic one-eye skeptical / side-eye expression.
static const RenderOp kEveSkepticalOps[] = {
    // Left eye — open
    {true, "LeftGlow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Left, false, false, {0, 0}, {74, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)42},
    {true, "LeftMid",  LayerOp::DrawMid,  Primitive::Ellipse, Side::Left, false, false, {0, 0}, {68, 108}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)110},
    {true, "LeftCore", LayerOp::DrawCore, Primitive::Ellipse, Side::Left, false, false, {0, 0}, {62, 104}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    // Right eye — skeptical squint
    {true, "RightGlow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Right, false, false, {0, 0}, {74, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)42},
    {true, "RightMid",  LayerOp::DrawMid,  Primitive::Ellipse, Side::Right, false, false, {0, 0}, {68, 108}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)110},
    {true, "RightCore", LayerOp::DrawCore, Primitive::Ellipse, Side::Right, false, false, {0, 0}, {62, 104}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "RightCut",  LayerOp::DrawCut,  Primitive::RoundedRect, Side::Right, false, false, {0, -34}, {92, 38}, 4, -14, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
    {true, "RightBrow", LayerOp::DrawBrow, Primitive::RoundedRect, Side::Right, false, false, {0, -62}, {54, 8},  4, -12, {ColorMode::RuntimeEye,       lv_color_hex(0x0fdaff), true},  LV_OPA_COVER},
};

static const EyeSceneSpec kEveSkepticalScene = {
    "eve_skeptical",
    {360, 360, 68, 180},
    lv_color_hex(0x06080d),
    kAnimFocused,
    kEveSkepticalOps,
    sizeof(kEveSkepticalOps) / sizeof(kEveSkepticalOps[0]),
};

} // namespace nse
