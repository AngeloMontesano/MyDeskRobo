#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveXXOps[] = {
    {true, "LeftGlow", LayerOp::DrawGlow, Primitive::RoundedRect, Side::Left, false, false, {0, 0}, {86, 86}, 12, 35, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)32},
    {true, "LeftBarA", LayerOp::DrawShape, Primitive::RoundedRect, Side::Left, false, false, {0, 0}, {74, 14}, 7, 35, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "LeftBarB", LayerOp::DrawShape, Primitive::RoundedRect, Side::Left, false, false, {0, 0}, {74, 14}, 7, -35, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "RightGlow", LayerOp::DrawGlow, Primitive::RoundedRect, Side::Right, false, false, {0, 0}, {86, 86}, 12, -35, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)32},
    {true, "RightBarA", LayerOp::DrawShape, Primitive::RoundedRect, Side::Right, false, false, {0, 0}, {74, 14}, 7, 35, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
    {true, "RightBarB", LayerOp::DrawShape, Primitive::RoundedRect, Side::Right, false, false, {0, 0}, {74, 14}, 7, -35, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
};

static const EyeSceneSpec kEveXXScene = {
    "eve_xx",
    {360, 360, 68, 180},
    lv_color_hex(0x06080d),
    kAnimStill,
    kEveXXOps,
    sizeof(kEveXXOps) / sizeof(kEveXXOps[0]),
};

} // namespace nse
