#pragma once

#include "SceneCommon.h"
#include "eve_happy.h"  // reuse kEveHappyOps

namespace nse {

// Same eye shape as HAPPY — tongue overlay is rendered as a separate LVGL widget
// in DeskRoboMVP_nextgen.cpp::update_tongue_overlay().
static const EyeSceneSpec kEveHappyTongueScene = {
    "eve_happy_tongue",
    {360, 360, 68, 180},
    lv_color_hex(0x06080d),
    kAnimIdle,
    kEveHappyOps,
    sizeof(kEveHappyOps) / sizeof(kEveHappyOps[0]),
};

} // namespace nse
