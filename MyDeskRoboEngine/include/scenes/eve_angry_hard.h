#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveAngryHardOps[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {74, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)42},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {68, 108}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)110},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {62, 104}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "Cut", LayerOp::DrawCut, Primitive::RoundedRect, Side::Both, true, true, {0, -45}, {107, 38}, 3, 31, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, LV_OPA_COVER},
    {true, "Brow",  LayerOp::DrawBrow, Primitive::RoundedRect, Side::Both, true, true, {8, -54}, {70, 11}, 3, 35, {ColorMode::RuntimeEye,      lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    // Pupil converges inward: mirror_x_for_right=true so side_x() flips pupil_x for right eye → both go toward nose.
    {true, "Pupil", LayerOp::DrawCut,  Primitive::Ellipse,     Side::Both, true, true, {0,   6}, {26, 32}, 13,  0, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, (lv_opa_t)120, true},
};

static const EyeSceneSpec kEveAngryHardScene = {
    "eve_angry_hard",
    {360, 360, 68, 180},
    lv_color_hex(0x06080d),
    kAnimExcited,
    kEveAngryHardOps,
    sizeof(kEveAngryHardOps) / sizeof(kEveAngryHardOps[0]),
};

} // namespace nse
