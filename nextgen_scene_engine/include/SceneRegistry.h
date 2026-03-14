#pragma once

#include "scenes/SceneCommon.h"
#include "scenes/eve_base.h"
#include "scenes/eve_idle.h"
#include "scenes/eve_happy.h"
#include "scenes/eve_sad.h"
#include "scenes/eve_angry_soft.h"
#include "scenes/eve_angry.h"
#include "scenes/eve_angry_hard.h"
#include "scenes/eve_wow.h"
#include "scenes/eve_sleepy.h"
#include "scenes/eve_confused.h"
#include "scenes/eve_excited.h"
#include "scenes/eve_dizzy.h"
#include "scenes/eve_call.h"
#include "scenes/eve_shake.h"

namespace nse {

static const NamedScene kBuiltinScenes[] = {
    {"eve_base", &kEveBaseScene},
    {"eve_idle", &kEveIdleScene},
    {"eve_happy", &kEveHappyScene},
    {"eve_sad", &kEveSadScene},
    {"eve_angry_soft", &kEveAngrySoftScene},
    {"eve_angry", &kEveAngryScene},
    {"eve_angry_hard", &kEveAngryHardScene},
    {"eve_wow", &kEveWowScene},
    {"eve_sleepy", &kEveSleepyScene},
    {"eve_confused", &kEveConfusedScene},
    {"eve_excited", &kEveExcitedScene},
    {"eve_dizzy", &kEveDizzyScene},
    {"eve_call", &kEveCallScene},
    {"eve_shake", &kEveShakeScene},
};

static constexpr size_t kBuiltinSceneCount = sizeof(kBuiltinScenes) / sizeof(kBuiltinScenes[0]);

static const NamedScene kBaseScenes[] = {
    {"eve_base", &kEveBaseScene},
};

static constexpr size_t kBaseSceneCount = sizeof(kBaseScenes) / sizeof(kBaseScenes[0]);

} // namespace nse
