#pragma once

#include "SceneSpec.h"

namespace nse {

static constexpr AnimationProfile kAnimStill = {
    {false, 4200, 140, 8},
    {false, 0, 0, 0, 0, 0, 0, 0, 0},
    {false, 0, 2400},
};

static constexpr AnimationProfile kAnimIdle = {
    {true, 4200, 140, 8},
    {true, 2, 2, 3, 2, 950, 1200, 2600, 3100},
    {false, 0, 2400},
};

static constexpr AnimationProfile kAnimCalm = {
    {true, 4600, 130, 8},
    {true, 1, 1, 2, 1, 1200, 1500, 3200, 3600},
    {false, 0, 2800},
};

static constexpr AnimationProfile kAnimFocused = {
    {true, 5200, 120, 8},
    {true, 1, 1, 1, 1, 1400, 1700, 3600, 4000},
    {false, 0, 3000},
};

static constexpr AnimationProfile kAnimExcited = {
    {true, 3000, 120, 10},
    {true, 3, 2, 4, 2, 820, 980, 2000, 2500},
    {false, 0, 1800},
};

static constexpr AnimationProfile kAnimWow = {
    {false, 3200, 90, 8},
    {false, 0, 0, 0, 0, 0, 0, 0, 0},
    {true, 8, 1500},
};

static constexpr AnimationProfile kAnimSleepy = {
    {true, 6200, 200, 6},
    {true, 1, 1, 1, 1, 1800, 2200, 4200, 4700},
    {false, 0, 3200},
};

static constexpr AnimationProfile kAnimSleepyStill = {
    {false, 6200, 200, 6},
    {true, 1, 1, 1, 1, 2100, 2500, 4800, 5200},
    {false, 0, 3400},
};

static constexpr AnimationProfile kAnimDizzy = {
    {true, 5200, 120, 8},
    {true, 4, 3, 5, 3, 700, 850, 1200, 1450},
    {false, 0, 1700},
};

static constexpr AnimationProfile kAnimShake = {
    {false, 3600, 140, 8},
    {true, 6, 4, 7, 4, 260, 340, 420, 500},
    {false, 0, 1200},
};

static constexpr AnimationProfile kAnimAlert = {
    {true, 3600, 110, 8},
    {true, 2, 1, 3, 1, 900, 1200, 2000, 2600},
    {false, 0, 2200},
};

} // namespace nse
