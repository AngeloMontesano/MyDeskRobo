#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveBaseAOps[] = {
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {62, 104}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
};

static const EyeSceneSpec kEveBaseAScene = {
    "eve_base_a_core_only",
    {360, 360, 68, 180},
    lv_color_hex(0x06080d),
    kAnimStill,
    kEveBaseAOps,
    sizeof(kEveBaseAOps) / sizeof(kEveBaseAOps[0]),
};

} // namespace nse
