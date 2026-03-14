#pragma once

#include "SceneCommon.h"

namespace nse {

static const RenderOp kEveBaseC_d1_s1_Ops[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {70, 108}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)40},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {64, 102}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)104},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {58, 98}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
};
static const EyeSceneSpec kEveBaseC_d1_s1_Scene = {"eve_c_d1_s1", {360, 360, 60, 180}, lv_color_hex(0x06080d), kAnimStill, kEveBaseC_d1_s1_Ops, sizeof(kEveBaseC_d1_s1_Ops) / sizeof(kEveBaseC_d1_s1_Ops[0])};

static const RenderOp kEveBaseC_d1_s2_Ops[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {74, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)42},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {68, 108}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)110},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {62, 104}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
};
static const EyeSceneSpec kEveBaseC_d1_s2_Scene = {"eve_c_d1_s2", {360, 360, 60, 180}, lv_color_hex(0x06080d), kAnimStill, kEveBaseC_d1_s2_Ops, sizeof(kEveBaseC_d1_s2_Ops) / sizeof(kEveBaseC_d1_s2_Ops[0])};

static const RenderOp kEveBaseC_d1_s3_Ops[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {78, 120}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)44},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {72, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)114},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {66, 110}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
};
static const EyeSceneSpec kEveBaseC_d1_s3_Scene = {"eve_c_d1_s3", {360, 360, 60, 180}, lv_color_hex(0x06080d), kAnimStill, kEveBaseC_d1_s3_Ops, sizeof(kEveBaseC_d1_s3_Ops) / sizeof(kEveBaseC_d1_s3_Ops[0])};

static const RenderOp kEveBaseC_d2_s1_Ops[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {70, 108}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)40},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {64, 102}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)104},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {58, 98}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
};
static const EyeSceneSpec kEveBaseC_d2_s1_Scene = {"eve_c_d2_s1", {360, 360, 68, 180}, lv_color_hex(0x06080d), kAnimStill, kEveBaseC_d2_s1_Ops, sizeof(kEveBaseC_d2_s1_Ops) / sizeof(kEveBaseC_d2_s1_Ops[0])};

static const RenderOp kEveBaseC_d2_s2_Ops[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {74, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)42},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {68, 108}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)110},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {62, 104}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
};
static const EyeSceneSpec kEveBaseC_d2_s2_Scene = {"eve_c_d2_s2", {360, 360, 68, 180}, lv_color_hex(0x06080d), kAnimStill, kEveBaseC_d2_s2_Ops, sizeof(kEveBaseC_d2_s2_Ops) / sizeof(kEveBaseC_d2_s2_Ops[0])};

static const RenderOp kEveBaseC_d2_s3_Ops[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {78, 120}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)44},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {72, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)114},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {66, 110}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
};
static const EyeSceneSpec kEveBaseC_d2_s3_Scene = {"eve_c_d2_s3", {360, 360, 68, 180}, lv_color_hex(0x06080d), kAnimStill, kEveBaseC_d2_s3_Ops, sizeof(kEveBaseC_d2_s3_Ops) / sizeof(kEveBaseC_d2_s3_Ops[0])};

static const RenderOp kEveBaseC_d3_s1_Ops[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {70, 108}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)40},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {64, 102}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)104},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {58, 98}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
};
static const EyeSceneSpec kEveBaseC_d3_s1_Scene = {"eve_c_d3_s1", {360, 360, 76, 180}, lv_color_hex(0x06080d), kAnimStill, kEveBaseC_d3_s1_Ops, sizeof(kEveBaseC_d3_s1_Ops) / sizeof(kEveBaseC_d3_s1_Ops[0])};

static const RenderOp kEveBaseC_d3_s2_Ops[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {74, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)42},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {68, 108}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)110},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {62, 104}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
};
static const EyeSceneSpec kEveBaseC_d3_s2_Scene = {"eve_c_d3_s2", {360, 360, 76, 180}, lv_color_hex(0x06080d), kAnimStill, kEveBaseC_d3_s2_Ops, sizeof(kEveBaseC_d3_s2_Ops) / sizeof(kEveBaseC_d3_s2_Ops[0])};

static const RenderOp kEveBaseC_d3_s3_Ops[] = {
    {true, "Glow", LayerOp::DrawGlow, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {78, 120}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)44},
    {true, "Mid", LayerOp::DrawMid, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {72, 114}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, (lv_opa_t)114},
    {true, "Core", LayerOp::DrawCore, Primitive::Ellipse, Side::Both, true, true, {0, 0}, {66, 110}, 999, 0, {ColorMode::RuntimeEye, lv_color_hex(0x0fdaff), true}, LV_OPA_COVER},
};
static const EyeSceneSpec kEveBaseC_d3_s3_Scene = {"eve_c_d3_s3", {360, 360, 76, 180}, lv_color_hex(0x06080d), kAnimStill, kEveBaseC_d3_s3_Ops, sizeof(kEveBaseC_d3_s3_Ops) / sizeof(kEveBaseC_d3_s3_Ops[0])};

} // namespace nse
