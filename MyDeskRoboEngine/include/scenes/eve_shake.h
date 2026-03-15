#pragma once

#include "SceneCommon.h"

namespace nse {

// Pupil jitter is driven by state.pupil_x/y — set per-frame in DeskRoboMVP_nextgen for SHAKE.
static const RenderOp kEveShakeOps[] = {
    {true, "Glow",  LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {74, 114}, 999, 0, {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true}, (lv_opa_t)42},
    {true, "Mid",   LayerOp::DrawMid,  Primitive::Ellipse, Side::Both, true, true, {0, 0}, {68, 108}, 999, 0, {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true}, (lv_opa_t)110},
    {true, "Core",  LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {62, 104}, 999, 0, {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "Pupil", LayerOp::DrawCut,  Primitive::Ellipse, Side::Both, true, true, {0, 0}, {28, 34},  14,  0, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, (lv_opa_t)120, true},
};

static const EyeSceneSpec kEveShakeScene = {
    "eve_shake",
    {360, 360, 68, 180},
    lv_color_hex(0x06080d),
    kAnimShake,
    kEveShakeOps,
    sizeof(kEveShakeOps) / sizeof(kEveShakeOps[0]),
};

} // namespace nse
