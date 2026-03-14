#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveAngryOps[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {74, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)42},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {68, 108}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)110},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {62, 104}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "Cut", LayerOp::DrawCut, Primitive::RoundedRect, Side::Both, true, true, {17, -46}, {101, 31}, 3, 22, {ColorMode::SceneBackground, lv_color_hex(0x06080d), false}, (lv_opa_t)251},
    {true, "Brow", LayerOp::DrawBrow, Primitive::RoundedRect, Side::Both, true, true, {0, -66}, {56, 8}, 3, 33, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "Glow2", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {74, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)42},
};

static const EyeSceneSpec kEveAngryScene = {
    "eve_angry",
    {360, 360, 68, 180},
    lv_color_hex(0x06080d),
    kAnimAlert,
    kEveAngryOps,
    sizeof(kEveAngryOps) / sizeof(kEveAngryOps[0]),
};

} // namespace nse
