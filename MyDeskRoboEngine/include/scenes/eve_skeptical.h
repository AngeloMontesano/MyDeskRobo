#pragma once

#include "SceneCommon.h"

namespace nse {

// Left eye: fully open with pupil.
// Right eye: squinted (outer-side cut) + matching brow + pupil in visible slit.
// → classic one-eye skeptical / side-eye expression.
static const RenderOp kEveSkepticalOps[] = {
    // Left eye — open
    {true, "LeftGlow",  LayerOp::DrawGlow, Primitive::Ellipse,     Side::Left,  false, false, {0, 0},   {74, 114}, 999, 0,  {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true},  (lv_opa_t)42},
    {true, "LeftMid",   LayerOp::DrawMid,  Primitive::Ellipse,     Side::Left,  false, false, {0, 0},   {68, 108}, 999, 0,  {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true},  (lv_opa_t)110},
    {true, "LeftCore",  LayerOp::DrawCore, Primitive::Ellipse,     Side::Left,  false, false, {0, 0},   {62, 104}, 999, 0,  {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true},  LV_OPA_COVER},
    {true, "LeftPupil", LayerOp::DrawCut,  Primitive::Ellipse,     Side::Left,  false, false, {0, 2},   {26, 32},  13, 0,  {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
    // Right eye — skeptical squint
    {true, "RightGlow",  LayerOp::DrawGlow, Primitive::Ellipse,     Side::Right, false, false, {0, 0},   {74, 114}, 999, 0,  {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true},  (lv_opa_t)42},
    {true, "RightMid",   LayerOp::DrawMid,  Primitive::Ellipse,     Side::Right, false, false, {0, 0},   {68, 108}, 999, 0,  {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true},  (lv_opa_t)110},
    {true, "RightCore",  LayerOp::DrawCore, Primitive::Ellipse,     Side::Right, false, false, {0, 0},   {62, 104}, 999, 0,  {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true},  LV_OPA_COVER},
    {true, "RightCut",   LayerOp::DrawCut,  Primitive::RoundedRect, Side::Right, false, false, {0, -36}, {92, 38},  4,  16, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
    {true, "RightBrow",  LayerOp::DrawBrow, Primitive::RoundedRect, Side::Right, false, false, {0, -64}, {54, 8},   4,  14, {ColorMode::RuntimeEye,       lv_color_hex(0x0fdaff), true},  LV_OPA_COVER},
    {true, "RightPupil", LayerOp::DrawCut,  Primitive::Ellipse,     Side::Right, false, false, {0, 16},  {22, 26},  11, 0,  {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
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
