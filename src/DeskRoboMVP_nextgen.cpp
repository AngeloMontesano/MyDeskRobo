#include "DeskRoboMVP.h"

#include <Preferences.h>
#include <lvgl.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "LVGL_Arduino/Display_ST77916.h"
#include "LayerRenderer.h"
#include "scenes/SceneCommon.h"
#include "scenes/eve_base.h"
#include "scenes/eve_idle.h"
#include "scenes/eve_happy.h"
#include "scenes/eve_sad.h"
#include "scenes/eve_angry_soft.h"
#include "scenes/eve_angry_hard.h"
#include "scenes/eve_wow.h"
#include "scenes/eve_sleepy.h"
#include "scenes/eve_wink.h"
#include "scenes/eve_xx.h"
#include "scenes/eve_confused.h"
#include "scenes/eve_shake.h"
#include "scenes/eve_call.h"
#include "scenes/eve_dizzy.h"
#include "scenes/eve_excited.h"
#include "scenes/eve_glitch.h"
#include "scenes/eve_skeptical.h"
#include "scenes/eve_bored.h"
#include "scenes/eve_focused.h"

using namespace nse;

namespace {

struct NextgenTuning {
  int drift_amp_px = 2;
  int saccade_amp_px = 3;
  int saccade_min_ms = 2600;
  int saccade_max_ms = 5200;
  int blink_interval_ms = 3600;
  int blink_duration_ms = 120;
  int double_blink_chance_pct = 20;
  int glow_pulse_amp = 9;
  int glow_pulse_period_ms = 2600;
  int shake_amp_px = 24;
  int shake_period_ms = 700;
  int sleep_delay_min = 10;
  int display_off_delay_min = 15;
  int eye_color_r = 15;
  int eye_color_g = 218;
  int eye_color_b = 255;
  int gyro_tilt_xy_pct = 62;
  int gyro_tilt_z_pct = 64;
  int gyro_tilt_cooldown_ms = 2200;
};

struct GlitchFxState {
  uint8_t row_frames = 0;
  uint8_t flicker_frames = 0;
  uint8_t split_frames = 0;
  uint8_t scan_frames = 0;
  int16_t row_y = 0;
  int16_t row_h = 0;
  int16_t row_offset = 0;
  lv_opa_t flicker_opa = LV_OPA_COVER;
  uint8_t scan_count = 0;
  int16_t scan_y[4] = {0, 0, 0, 0};
  int16_t scan_h[4] = {0, 0, 0, 0};
};

struct SaccadeState {
  int16_t target_x = 0;
  int16_t target_y = 0;
  uint32_t hold_until_ms = 0;
};

struct GazeState {
  int16_t target_x = 0;
  int16_t target_y = 0;
  uint32_t hold_until_ms = 0;
  uint32_t next_gaze_ms = 0;
};

// Gaze directions: left/right + 4 diagonals only.
// Pure up/down removed — doesn't read well on this tall ellipse eye shape.
static const int8_t kGazeDirX[] = { 1, -1,  1,  1, -1, -1};
static const int8_t kGazeDirY[] = { 0,  0, -1,  1,  1, -1};

LayerRenderer *g_left_renderer = nullptr;
LayerRenderer *g_right_renderer = nullptr;
lv_obj_t *g_left_canvas = nullptr;
lv_obj_t *g_right_canvas = nullptr;
lv_obj_t *g_status_label = nullptr;
lv_obj_t *g_scene_label = nullptr;
lv_obj_t *g_sleep_labels[3] = {nullptr, nullptr, nullptr};
lv_obj_t *g_confused_label = nullptr;
lv_obj_t *g_boot_splash = nullptr;
uint32_t g_boot_splash_started_ms = 0;

Preferences g_prefs;
NextgenTuning g_tuning;
GlitchFxState g_glitch_fx;
SaccadeState g_saccade;
GazeState g_gaze;
uint32_t g_next_blink_interval_ms = 3600;
uint32_t g_next_rr_interval_ms = 15000;
uint32_t g_micro_expr_next_ms = 0;

DeskRoboEmotion g_current_emotion = DESKROBO_EMOTION_IDLE;
DeskRoboEmotion g_left_eye_emotion = DESKROBO_EMOTION_IDLE;
DeskRoboEmotion g_right_eye_emotion = DESKROBO_EMOTION_IDLE;
DeskRoboFaceStyle g_style = DESKROBO_STYLE_EVE;
uint8_t g_current_priority = 0;
uint32_t g_emotion_expiry_ms = 0;
uint32_t g_eye_pair_expiry_ms = 0;
uint32_t g_last_interaction_ms = 0;
uint32_t g_last_idle_round_ms = 0;
uint32_t g_last_scene_cycle_ms = 0;
uint32_t g_last_blink_toggle_ms = 0;
bool g_blink_active = false;
bool g_double_blink_pending = false;
bool g_eye_pair_active = false;
bool g_status_label_visible = false;
bool g_ble_connected = false;
bool g_display_sleep_off = false;
bool g_display_dimmed = false;
uint8_t g_backlight_before_sleep = 15;
uint8_t g_idle_round_index = 0;

static constexpr uint32_t kIdleRoundRobinAfterMs = 20UL * 1000UL;
static constexpr uint32_t kIdleRoundRobinIntervalMs = 15UL * 1000UL;
static constexpr uint32_t kIdleRoundRobinShowMs = 8000UL;
static constexpr uint8_t kIdleRoundRobinPriority = 5;
static constexpr uint8_t kScreensaverDimPct = 35;
static constexpr uint32_t kBootSplashMs = 2800UL;
static constexpr const char *kBootBrand = "My Robo Desk";
static constexpr const char *kBootVersion = "v0.5";
// Mutable shuffled idle sequence. GLITCH is always immediately followed by CONFUSED.
static constexpr uint8_t kIdleRoundLen = 12;
static DeskRoboEmotion g_idle_round[kIdleRoundLen] = {
    DESKROBO_EMOTION_HAPPY,
    DESKROBO_EMOTION_SAD,
    DESKROBO_EMOTION_ANGRY,
    DESKROBO_EMOTION_WOW,
    DESKROBO_EMOTION_SLEEPY,
    DESKROBO_EMOTION_WINK,
    DESKROBO_EMOTION_XX,
    DESKROBO_EMOTION_SKEPTICAL,
    DESKROBO_EMOTION_BORED,
    DESKROBO_EMOTION_FOCUSED,
    DESKROBO_EMOTION_GLITCH,
    DESKROBO_EMOTION_CONFUSED,
};

void reset_blink_state();
void reset_glitch_fx();
void shuffle_idle_round();

const char *emotion_name(DeskRoboEmotion emotion) {
  switch (emotion) {
    case DESKROBO_EMOTION_HAPPY: return "HAPPY";
    case DESKROBO_EMOTION_SAD: return "SAD";
    case DESKROBO_EMOTION_ANGRY: return "ANGRY";
    case DESKROBO_EMOTION_ANGRY_SOFT: return "ANGRY_SOFT";
    case DESKROBO_EMOTION_ANGRY_HARD: return "ANGRY_HARD";
    case DESKROBO_EMOTION_WOW: return "WOW";
    case DESKROBO_EMOTION_SLEEPY: return "SLEEPY";
    case DESKROBO_EMOTION_CONFUSED: return "CONFUSED";
    case DESKROBO_EMOTION_EXCITED: return "EXCITED";
    case DESKROBO_EMOTION_DIZZY: return "DIZZY";
    case DESKROBO_EMOTION_MAIL: return "MAIL";
    case DESKROBO_EMOTION_CALL: return "CALL";
    case DESKROBO_EMOTION_SHAKE: return "SHAKE";
    case DESKROBO_EMOTION_WINK: return "WINK";
    case DESKROBO_EMOTION_XX: return "XX";
    case DESKROBO_EMOTION_GLITCH: return "GLITCH";
    case DESKROBO_EMOTION_SKEPTICAL: return "SKEPTICAL";
    case DESKROBO_EMOTION_BORED: return "BORED";
    case DESKROBO_EMOTION_FOCUSED: return "FOCUSED";
    default: return "IDLE";
  }
}

lv_color_t eye_color() {
  return lv_color_make((uint8_t)constrain(g_tuning.eye_color_r, 0, 255),
                       (uint8_t)constrain(g_tuning.eye_color_g, 0, 255),
                       (uint8_t)constrain(g_tuning.eye_color_b, 0, 255));
}

bool wifi_session_active() {
  return false;
}

uint8_t wake_backlight_level() {
  return g_backlight_before_sleep > 0 ? g_backlight_before_sleep : 15;
}

void wake_display() {
  if (!g_display_sleep_off && !g_display_dimmed) return;
  Set_Backlight(wake_backlight_level());
  g_display_sleep_off = false;
  g_display_dimmed = false;
}

void mark_interaction(uint32_t now) {
  g_last_interaction_ms = now;
  wake_display();
}

const EyeSceneSpec &scene_for_emotion(DeskRoboEmotion emotion, DeskRoboFaceStyle style) {
  (void)style;
  switch (emotion) {
    case DESKROBO_EMOTION_HAPPY: return kEveHappyScene;
    case DESKROBO_EMOTION_SAD: return kEveSadScene;
    case DESKROBO_EMOTION_ANGRY_SOFT: return kEveAngrySoftScene;
    case DESKROBO_EMOTION_ANGRY_HARD: return kEveAngryHardScene;
    case DESKROBO_EMOTION_ANGRY: return kEveAngryHardScene;
    case DESKROBO_EMOTION_WOW: return kEveWowScene;
    case DESKROBO_EMOTION_SLEEPY: return kEveSleepyScene;
    case DESKROBO_EMOTION_CONFUSED: return kEveConfusedScene;
    case DESKROBO_EMOTION_EXCITED: return kEveExcitedScene;
    case DESKROBO_EMOTION_DIZZY: return kEveDizzyScene;
    case DESKROBO_EMOTION_MAIL: return kEveConfusedScene;
    case DESKROBO_EMOTION_CALL: return kEveCallScene;
    case DESKROBO_EMOTION_SHAKE: return kEveShakeScene;
    case DESKROBO_EMOTION_WINK: return kEveWinkScene;
    case DESKROBO_EMOTION_XX: return kEveXXScene;
    case DESKROBO_EMOTION_GLITCH: return kEveGlitchScene;
    case DESKROBO_EMOTION_SKEPTICAL: return kEveSkepticalScene;
    case DESKROBO_EMOTION_BORED: return kEveBoredScene;
    case DESKROBO_EMOTION_FOCUSED: return kEveFocusedScene;
    case DESKROBO_EMOTION_IDLE:
    default:
      return kEveIdleScene;
  }
}

bool is_glitch_scene(const EyeSceneSpec &scene) {
  return strcmp(scene.name, kEveGlitchScene.name) == 0;
}

void update_status_label() {
  if (!g_status_label) return;
  if (!g_status_label_visible) {
    lv_obj_add_flag(g_status_label, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_clear_flag(g_status_label, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text_fmt(g_status_label, "MyDeskRobo: %s", emotion_name(g_current_emotion));
}

void update_scene_label(const char *scene_name) {
  if (!g_scene_label) return;
  const bool show = scene_name && strstr(scene_name, "render_fail");
  if (!show) {
    lv_obj_add_flag(g_scene_label, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_clear_flag(g_scene_label, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text_fmt(g_scene_label, "%s", scene_name);
}

void update_boot_splash() {
  if (!g_boot_splash) return;
  if ((millis() - g_boot_splash_started_ms) < kBootSplashMs) {
    lv_obj_clear_flag(g_boot_splash, LV_OBJ_FLAG_HIDDEN);
    return;
  }

  lv_obj_del(g_boot_splash);
  g_boot_splash = nullptr;
  g_current_emotion = DESKROBO_EMOTION_IDLE;
  g_left_eye_emotion = DESKROBO_EMOTION_IDLE;
  g_right_eye_emotion = DESKROBO_EMOTION_IDLE;
  g_eye_pair_active = false;
  g_current_priority = 0;
  g_emotion_expiry_ms = 0;
  g_eye_pair_expiry_ms = 0;
  g_last_idle_round_ms = millis();
  reset_blink_state();
  reset_glitch_fx();
}

void reset_blink_state() {
  g_last_blink_toggle_ms = millis();
  g_blink_active = false;
  g_double_blink_pending = false;
}

void reset_glitch_fx() {
  g_glitch_fx = GlitchFxState{};
}

void shuffle_idle_round() {
  // Base pool: all emotions except GLITCH and CONFUSED (those are a fixed pair)
  DeskRoboEmotion base[] = {
      DESKROBO_EMOTION_HAPPY,
      DESKROBO_EMOTION_SAD,
      DESKROBO_EMOTION_ANGRY,
      DESKROBO_EMOTION_WOW,
      DESKROBO_EMOTION_SLEEPY,
      DESKROBO_EMOTION_WINK,
      DESKROBO_EMOTION_XX,
      DESKROBO_EMOTION_SKEPTICAL,
      DESKROBO_EMOTION_BORED,
      DESKROBO_EMOTION_FOCUSED,
  };
  const uint8_t base_len = 10;

  // Fisher-Yates shuffle of base pool
  for (uint8_t i = base_len - 1; i > 0; --i) {
    const uint8_t j = (uint8_t)random(i + 1);
    const DeskRoboEmotion tmp = base[i];
    base[i] = base[j];
    base[j] = tmp;
  }

  // Insert GLITCH+CONFUSED pair at a random position
  const uint8_t insert_pos = (uint8_t)random(base_len + 1);
  uint8_t out = 0;
  for (uint8_t i = 0; i < base_len; ++i) {
    if (i == insert_pos) {
      g_idle_round[out++] = DESKROBO_EMOTION_GLITCH;
      g_idle_round[out++] = DESKROBO_EMOTION_CONFUSED;
    }
    g_idle_round[out++] = base[i];
  }
  if (insert_pos == base_len) {
    g_idle_round[out++] = DESKROBO_EMOTION_GLITCH;
    g_idle_round[out++] = DESKROBO_EMOTION_CONFUSED;
  }
}

int16_t wave_value(uint32_t now_ms, uint16_t period_ms, int16_t amplitude, bool use_cos) {
  if (period_ms == 0 || amplitude == 0) return 0;
  const float phase = (float)(now_ms % period_ms) / (float)period_ms;
  const float angle = phase * 6.2831853f;
  const float value = use_cos ? cosf(angle) : sinf(angle);
  return (int16_t)(value * (float)amplitude);
}

RuntimeState make_runtime_state(const EyeSceneSpec &scene, lv_color_t color, bool include_glitch) {
  const uint32_t now = millis();
  if (!g_blink_active && (now - g_last_blink_toggle_ms) > g_next_blink_interval_ms) {
    g_blink_active = true;
    g_last_blink_toggle_ms = now;
  } else if (g_blink_active && (now - g_last_blink_toggle_ms) > (uint32_t)g_tuning.blink_duration_ms) {
    g_blink_active = false;
    g_last_blink_toggle_ms = now;
    // Jitter: ±20% around blink_interval_ms so blinks don't feel mechanical
    const int32_t jitter = (int32_t)random(-(g_tuning.blink_interval_ms / 5), g_tuning.blink_interval_ms / 5 + 1);
    const int32_t next_blink = g_tuning.blink_interval_ms + jitter;
    g_next_blink_interval_ms = (uint32_t)(next_blink < 600 ? 600 : next_blink);
    if (!g_double_blink_pending && random(100) < g_tuning.double_blink_chance_pct) {
      g_double_blink_pending = true;
    } else {
      g_double_blink_pending = false;
    }
  } else if (g_double_blink_pending && !g_blink_active && (now - g_last_blink_toggle_ms) > 120U) {
    g_double_blink_pending = false;
    g_blink_active = true;
    g_last_blink_toggle_ms = now;
  }

  RuntimeState state{};
  state.eye_color = color;
  state.blink = g_blink_active;
  state.blink_height = scene.animation.blink.closed_height > 0 ? scene.animation.blink.closed_height : 8;
  state.output_opa = LV_OPA_COVER;

  int16_t drift_amp = g_tuning.drift_amp_px;
  int16_t saccade_amp = g_tuning.saccade_amp_px;
  uint16_t drift_x_period = 950;
  uint16_t drift_y_period = 1200;

  // Sleepy transition: gradually slow animation as device approaches sleep
  {
    const uint32_t idle_ms = now - g_last_interaction_ms;
    const uint32_t sleep_ms = (uint32_t)g_tuning.sleep_delay_min * 60000UL;
    const uint32_t onset_ms = sleep_ms * 6 / 10;
    if (!g_ble_connected && sleep_ms > 0 && idle_ms > onset_ms) {
      const float t = (float)(idle_ms - onset_ms) / (float)(sleep_ms - onset_ms);
      const float f = 1.0f - (t > 1.0f ? 1.0f : t) * 0.65f;
      drift_amp  = (int16_t)((float)drift_amp  * f);
      saccade_amp = (int16_t)((float)saccade_amp * f);
      drift_x_period = (uint16_t)(drift_x_period  * (1.0f + (1.0f - f)));
      drift_y_period = (uint16_t)(drift_y_period  * (1.0f + (1.0f - f)));
    }
  }

  if (strcmp(scene.name, "eve_shake") == 0) {
    drift_amp = max(6, g_tuning.shake_amp_px / 2);
    saccade_amp = 0;
    drift_x_period = (uint16_t)max(360, g_tuning.shake_period_ms + 80);
    drift_y_period = (uint16_t)max(460, g_tuning.shake_period_ms + 180);
  }
  state.drift_x = wave_value(now, drift_x_period, drift_amp, false);
  state.drift_y = wave_value(now, drift_y_period, drift_amp / 2, true);

  if (saccade_amp > 0) {
    if (now >= g_saccade.hold_until_ms) {
      g_saccade.target_x = (int16_t)random(-saccade_amp, saccade_amp + 1);
      g_saccade.target_y = (int16_t)random(-(saccade_amp / 2), (saccade_amp / 2) + 1);
      g_saccade.hold_until_ms = now + (uint32_t)random(g_tuning.saccade_min_ms, g_tuning.saccade_max_ms + 1);
    }
    // Gaze: occasionally look clearly in one of 6 directions.
    // CONFUSED Denkpose: 70% chance to look upper-left (classic thinking gaze).
    if (now >= g_gaze.next_gaze_ms && now >= g_gaze.hold_until_ms) {
      if (strcmp(scene.name, "eve_confused") == 0 && random(10) < 7) {
        g_gaze.target_x = (int16_t)(kGazeDirX[5] * 11); // upper-left
        g_gaze.target_y = (int16_t)(kGazeDirY[5] * 7);
      } else {
        const uint8_t pick = (uint8_t)random(6);
        g_gaze.target_x = (int16_t)(kGazeDirX[pick] * 11);
        g_gaze.target_y = (int16_t)(kGazeDirY[pick] * 7);
      }
      g_gaze.hold_until_ms = now + (uint32_t)random(1200, 2800);
      g_gaze.next_gaze_ms = g_gaze.hold_until_ms + (uint32_t)random(7000, 18000);
      // Blink just before shifting gaze, like real eyes do
      if (!g_blink_active) {
        g_blink_active = true;
        g_last_blink_toggle_ms = now;
      }
    }
    if (now < g_gaze.hold_until_ms) {
      state.saccade_x = g_gaze.target_x;
      state.saccade_y = g_gaze.target_y;
    } else {
      state.saccade_x = g_saccade.target_x;
      state.saccade_y = g_saccade.target_y;
    }
  } else {
    state.saccade_x = 0;
    state.saccade_y = 0;
  }
  if (g_tuning.glow_pulse_period_ms > 0) {
    const float phase = (float)(now % (uint32_t)g_tuning.glow_pulse_period_ms) / (float)g_tuning.glow_pulse_period_ms;
    state.pulse_shift = (int8_t)(sinf(phase * 6.2831853f) * (float)g_tuning.glow_pulse_amp);
  }

  if (include_glitch && is_glitch_scene(scene)) {
    if (g_glitch_fx.flicker_frames > 0) state.output_opa = g_glitch_fx.flicker_opa;
    if (g_glitch_fx.row_frames > 0) {
      state.glitch_row_shift = true;
      state.glitch_row_y = g_glitch_fx.row_y;
      state.glitch_row_h = g_glitch_fx.row_h;
      state.glitch_row_offset = g_glitch_fx.row_offset;
    }
    if (g_glitch_fx.scan_frames > 0) {
      state.glitch_scanline_count = g_glitch_fx.scan_count;
      for (uint8_t i = 0; i < g_glitch_fx.scan_count && i < 4; ++i) {
        state.glitch_scanline_y[i] = g_glitch_fx.scan_y[i];
        state.glitch_scanline_h[i] = g_glitch_fx.scan_h[i];
      }
    }
  }
  return state;
}

void update_glitch_fx(const EyeSceneSpec &scene) {
  if (!is_glitch_scene(scene)) {
    reset_glitch_fx();
    return;
  }

  if (g_glitch_fx.row_frames > 0) g_glitch_fx.row_frames--;
  else if (random(100) < 15) {
    g_glitch_fx.row_frames = (uint8_t)random(2, 6);
    g_glitch_fx.row_y = (int16_t)random(70, 260);
    g_glitch_fx.row_h = (int16_t)random(1, 4);
    const int16_t mag = (int16_t)random(5, 16);
    g_glitch_fx.row_offset = random(2) == 0 ? -mag : mag;
  }
  if (g_glitch_fx.flicker_frames > 0) g_glitch_fx.flicker_frames--;
  else if (random(100) < 5) {
    g_glitch_fx.flicker_frames = (uint8_t)random(1, 3);
    g_glitch_fx.flicker_opa = (lv_opa_t)random(120, 181);
  }
  if (g_glitch_fx.split_frames > 0) g_glitch_fx.split_frames--;
  else if (random(100) < 14) {
    g_glitch_fx.split_frames = (uint8_t)random(2, 6);
  }
  if (g_glitch_fx.scan_frames > 0) g_glitch_fx.scan_frames--;
  else if (random(100) < 20) {
    g_glitch_fx.scan_frames = (uint8_t)random(3, 7);
    g_glitch_fx.scan_count = (uint8_t)random(3, 5);
    for (uint8_t i = 0; i < g_glitch_fx.scan_count && i < 4; ++i) {
      g_glitch_fx.scan_y[i] = (int16_t)random(30, 330);
      g_glitch_fx.scan_h[i] = (int16_t)random(4, 10);
    }
  }

}

void update_sleep_overlay(const EyeSceneSpec &left_scene, const EyeSceneSpec &right_scene) {
  const bool show = strcmp(left_scene.name, "eve_sleepy") == 0 || strcmp(right_scene.name, "eve_sleepy") == 0;
  const uint32_t now = millis();
  const lv_coord_t base_x[3] = {184, 226, 268};
  const lv_coord_t base_y[3] = {96, 68, 42};
  for (int i = 0; i < 3; ++i) {
    if (!g_sleep_labels[i]) continue;
    if (!show) {
      lv_obj_add_flag(g_sleep_labels[i], LV_OBJ_FLAG_HIDDEN);
      continue;
    }
    lv_obj_clear_flag(g_sleep_labels[i], LV_OBJ_FLAG_HIDDEN);
    const float phase = ((float)(now % (2200U + i * 280U)) / (float)(2200U + i * 280U)) * 6.2831853f;
    const lv_coord_t dy = (lv_coord_t)(sinf(phase) * (4 + i * 2));
    const lv_coord_t dx = (lv_coord_t)(cosf(phase * 0.7f) * (2 + i));
    lv_obj_set_pos(g_sleep_labels[i], base_x[i] + dx, base_y[i] + dy);
  }
}

void update_confused_overlay(const EyeSceneSpec &left_scene, const EyeSceneSpec &right_scene) {
  if (!g_confused_label) return;
  const bool show = strcmp(left_scene.name, "eve_confused") == 0 || strcmp(right_scene.name, "eve_confused") == 0;
  if (!show) {
    lv_obj_add_flag(g_confused_label, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_clear_flag(g_confused_label, LV_OBJ_FLAG_HIDDEN);
  const uint32_t now = millis();
  const float phase = ((float)(now % 2400U) / 2400.0f) * 6.2831853f;
  lv_obj_set_pos(g_confused_label, 250 + (lv_coord_t)(cosf(phase * 0.6f) * 2), 28 + (lv_coord_t)(sinf(phase) * 4));
}

void apply_sleep_policy() {
  if (g_ble_connected || wifi_session_active()) {
    wake_display();
    return;
  }
  const uint32_t now = millis();
  const uint32_t idle_ms = now - g_last_interaction_ms;
  const uint32_t sleep_ms = (uint32_t)g_tuning.sleep_delay_min * 60UL * 1000UL;
  const uint32_t off_ms = (uint32_t)max(g_tuning.display_off_delay_min, g_tuning.sleep_delay_min) * 60UL * 1000UL;
  if (idle_ms < sleep_ms) return;

  if (!g_display_dimmed && !g_display_sleep_off) {
    if (LCD_Backlight > 0) g_backlight_before_sleep = LCD_Backlight;
    Set_Backlight((uint8_t)((wake_backlight_level() * kScreensaverDimPct) / 100U));
    g_display_dimmed = true;
  }
  if (idle_ms >= off_ms && !g_display_sleep_off) {
    if (LCD_Backlight > 0) g_backlight_before_sleep = LCD_Backlight;
    Set_Backlight(0);
    g_display_sleep_off = true;
  }
}

void maybe_micro_expression() {
  if (g_eye_pair_active || g_current_priority > 0 || g_boot_splash) return;
  const uint32_t now = millis();
  if ((now - g_last_interaction_ms) < kIdleRoundRobinAfterMs) return;
  if (now < g_micro_expr_next_ms) return;
  static const DeskRoboEmotion kMicroPool[] = {
      DESKROBO_EMOTION_HAPPY,
      DESKROBO_EMOTION_CONFUSED,
      DESKROBO_EMOTION_ANGRY_SOFT,
      DESKROBO_EMOTION_WOW,
  };
  const uint8_t pick = (uint8_t)random(sizeof(kMicroPool) / sizeof(kMicroPool[0]));
  g_current_emotion = kMicroPool[pick];
  g_current_priority = 3;
  g_emotion_expiry_ms = now + (uint32_t)random(280, 560);
  g_micro_expr_next_ms = now + (uint32_t)random(120000UL, 300000UL);
}

void apply_pre_sleep_emotion() {
  if (g_ble_connected || wifi_session_active()) return;
  if (g_current_priority > 2 || g_boot_splash) return;
  const uint32_t now = millis();
  const uint32_t idle_ms = now - g_last_interaction_ms;
  const uint32_t sleep_ms = (uint32_t)g_tuning.sleep_delay_min * 60000UL;
  if (sleep_ms == 0 || idle_ms < sleep_ms * 6 / 10) return;
  g_current_emotion = DESKROBO_EMOTION_SLEEPY;
  g_current_priority = 2;
  g_emotion_expiry_ms = now + 5000;
}

void maybe_idle_round_robin() {
  const uint32_t now = millis();
  if (g_eye_pair_active || g_current_priority > 0) return;
  if ((now - g_last_interaction_ms) < kIdleRoundRobinAfterMs) return;
  if ((now - g_last_idle_round_ms) < g_next_rr_interval_ms) return;
  g_last_idle_round_ms = now;
  g_idle_round_index = (uint8_t)(g_idle_round_index + 1);
  if (g_idle_round_index >= kIdleRoundLen) {
    g_idle_round_index = 0;
    shuffle_idle_round();
  }
  const DeskRoboEmotion rr_emotion = g_idle_round[g_idle_round_index];
  const uint32_t rr_show_ms = (rr_emotion == DESKROBO_EMOTION_WINK)
      ? (uint32_t)random(400, 650)
      : kIdleRoundRobinShowMs;
  g_current_emotion = rr_emotion;
  g_current_priority = kIdleRoundRobinPriority;
  g_emotion_expiry_ms = now + rr_show_ms;
  g_next_rr_interval_ms = (uint32_t)random(10000, 22000);
}

void maybe_expire_states() {
  const uint32_t now = millis();
  if (g_eye_pair_active && now >= g_eye_pair_expiry_ms) {
    g_eye_pair_active = false;
    g_left_eye_emotion = DESKROBO_EMOTION_IDLE;
    g_right_eye_emotion = DESKROBO_EMOTION_IDLE;
  }
  if (g_current_priority > 0 && now >= g_emotion_expiry_ms) {
    g_current_priority = 0;
    g_current_emotion = DESKROBO_EMOTION_IDLE;
  }
}

void init_canvas(lv_obj_t *canvas, lv_coord_t x, lv_coord_t width) {
  lv_obj_remove_style_all(canvas);
  lv_obj_clear_flag(canvas, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(canvas, width, kEyeViewportHeight);
  lv_obj_set_pos(canvas, x, 86);
  lv_obj_set_style_bg_color(canvas, lv_color_hex(0x06080D), 0);
  lv_obj_set_style_bg_opa(canvas, LV_OPA_COVER, 0);
}

void create_ui() {
  const lv_coord_t half_w = EXAMPLE_LCD_WIDTH / 2;

  g_left_canvas = lv_canvas_create(lv_scr_act());
  init_canvas(g_left_canvas, 0, half_w);

  g_right_canvas = lv_canvas_create(lv_scr_act());
  init_canvas(g_right_canvas, half_w, half_w);

  g_scene_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(g_scene_label, lv_color_make(0x6A, 0xD7, 0xF5), 0);
  lv_obj_set_style_text_font(g_scene_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_bg_color(g_scene_label, lv_color_hex(0x06080D), 0);
  lv_obj_set_style_bg_opa(g_scene_label, LV_OPA_50, 0);
  lv_obj_set_style_pad_left(g_scene_label, 8, 0);
  lv_obj_set_style_pad_right(g_scene_label, 8, 0);
  lv_obj_set_style_pad_top(g_scene_label, 4, 0);
  lv_obj_set_style_pad_bottom(g_scene_label, 4, 0);
  lv_obj_set_style_radius(g_scene_label, 10, 0);
  lv_obj_align(g_scene_label, LV_ALIGN_TOP_MID, 0, 12);
  lv_obj_add_flag(g_scene_label, LV_OBJ_FLAG_HIDDEN);

  g_status_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(g_status_label, lv_color_make(0xD7, 0xEE, 0xFF), 0);
  lv_obj_set_style_text_font(g_status_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_bg_color(g_status_label, lv_color_hex(0x06080D), 0);
  lv_obj_set_style_bg_opa(g_status_label, LV_OPA_40, 0);
  lv_obj_set_style_pad_left(g_status_label, 6, 0);
  lv_obj_set_style_pad_right(g_status_label, 6, 0);
  lv_obj_set_style_pad_top(g_status_label, 3, 0);
  lv_obj_set_style_pad_bottom(g_status_label, 3, 0);
  lv_obj_set_style_radius(g_status_label, 8, 0);
  lv_obj_align(g_status_label, LV_ALIGN_BOTTOM_MID, 0, -12);
  lv_obj_add_flag(g_status_label, LV_OBJ_FLAG_HIDDEN);

  const char *sleep_texts[3] = {"ZZZ", "ZZ", "Z"};
  const lv_coord_t sleep_x[3] = {184, 226, 268};
  const lv_coord_t sleep_y[3] = {96, 68, 42};
  for (int i = 0; i < 3; ++i) {
    g_sleep_labels[i] = lv_label_create(lv_scr_act());
    lv_label_set_text(g_sleep_labels[i], sleep_texts[i]);
    lv_obj_set_style_text_color(g_sleep_labels[i], lv_color_make(0x0F, 0xDA, 0xFF), 0);
    lv_obj_set_style_text_opa(g_sleep_labels[i], LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(g_sleep_labels[i], &lv_font_montserrat_16, 0);
    lv_obj_set_style_bg_opa(g_sleep_labels[i], LV_OPA_TRANSP, 0);
    lv_obj_add_flag(g_sleep_labels[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(g_sleep_labels[i], sleep_x[i], sleep_y[i]);
  }

  g_confused_label = lv_label_create(lv_scr_act());
  lv_label_set_text(g_confused_label, "??");
  lv_obj_set_style_text_color(g_confused_label, lv_color_make(0x0F, 0xDA, 0xFF), 0);
  lv_obj_set_style_text_opa(g_confused_label, LV_OPA_COVER, 0);
  lv_obj_set_style_text_font(g_confused_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_bg_opa(g_confused_label, LV_OPA_TRANSP, 0);
  lv_obj_add_flag(g_confused_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_pos(g_confused_label, 248, 24);

  g_boot_splash = lv_obj_create(lv_scr_act());
  lv_obj_remove_style_all(g_boot_splash);
  lv_obj_clear_flag(g_boot_splash, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(g_boot_splash, EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT);
  lv_obj_set_style_bg_color(g_boot_splash, lv_color_hex(0x06080D), 0);
  lv_obj_set_style_bg_opa(g_boot_splash, LV_OPA_COVER, 0);
  lv_obj_center(g_boot_splash);

  lv_obj_t *logo = lv_obj_create(g_boot_splash);
  lv_obj_remove_style_all(logo);
  lv_obj_set_size(logo, 88, 88);
  lv_obj_set_style_radius(logo, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(logo, lv_color_hex(0x0A1017), 0);
  lv_obj_set_style_bg_opa(logo, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(logo, 2, 0);
  lv_obj_set_style_border_color(logo, lv_color_make(0x0F, 0xDA, 0xFF), 0);
  lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 62);

  lv_obj_t *eye_l = lv_obj_create(logo);
  lv_obj_remove_style_all(eye_l);
  lv_obj_set_size(eye_l, 16, 28);
  lv_obj_set_style_radius(eye_l, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(eye_l, lv_color_make(0x0F, 0xDA, 0xFF), 0);
  lv_obj_set_style_bg_opa(eye_l, LV_OPA_COVER, 0);
  lv_obj_align(eye_l, LV_ALIGN_CENTER, -14, 0);

  lv_obj_t *eye_r = lv_obj_create(logo);
  lv_obj_remove_style_all(eye_r);
  lv_obj_set_size(eye_r, 16, 28);
  lv_obj_set_style_radius(eye_r, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(eye_r, lv_color_make(0x0F, 0xDA, 0xFF), 0);
  lv_obj_set_style_bg_opa(eye_r, LV_OPA_COVER, 0);
  lv_obj_align(eye_r, LV_ALIGN_CENTER, 14, 0);

  lv_obj_t *title = lv_label_create(g_boot_splash);
  lv_label_set_text(title, kBootBrand);
  lv_obj_set_style_text_color(title, lv_color_make(0xD7, 0xEE, 0xFF), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 176);

  lv_obj_t *version = lv_label_create(g_boot_splash);
  lv_label_set_text(version, kBootVersion);
  lv_obj_set_style_text_color(version, lv_color_make(0x6A, 0xD7, 0xF5), 0);
  lv_obj_set_style_text_font(version, &lv_font_montserrat_16, 0);
  lv_obj_align(version, LV_ALIGN_TOP_MID, 0, 212);

  static LayerRenderer left_renderer(g_left_canvas);
  static LayerRenderer right_renderer(g_right_canvas);
  g_left_renderer = &left_renderer;
  g_right_renderer = &right_renderer;
}

void render_active() {
  const DeskRoboEmotion left_emotion = g_eye_pair_active ? g_left_eye_emotion : g_current_emotion;
  const DeskRoboEmotion right_emotion = g_eye_pair_active ? g_right_eye_emotion : g_current_emotion;
  const EyeSceneSpec &left_scene = scene_for_emotion(left_emotion, g_style);
  const EyeSceneSpec &right_scene = scene_for_emotion(right_emotion, g_style);
  update_scene_label(g_eye_pair_active ? "eye_pair" : left_scene.name);
  update_glitch_fx(left_scene);
  update_sleep_overlay(left_scene, right_scene);
  update_confused_overlay(left_scene, right_scene);

  lv_color_t left_color = eye_color();
  lv_color_t right_color = left_color;
  const bool glitch_active = (g_glitch_fx.row_frames > 0) ||
                             (g_glitch_fx.flicker_frames > 0) ||
                             (g_glitch_fx.scan_frames > 0);
  if (is_glitch_scene(left_scene) && glitch_active) {
    left_color = lv_color_make(0xFF, 0x00, 0x40);
  }
  if (is_glitch_scene(right_scene) && glitch_active) {
    right_color = lv_color_make(0xFF, 0x00, 0x40);
  }

  RuntimeState left_state = make_runtime_state(left_scene, left_color, is_glitch_scene(left_scene));
  RuntimeState right_state = make_runtime_state(right_scene, right_color, is_glitch_scene(right_scene));
  const bool left_ok = g_left_renderer && g_left_renderer->render_eye(left_scene, left_state, false);
  const bool right_ok = g_right_renderer && g_right_renderer->render_eye(right_scene, right_state, true);
  if (!left_ok || !right_ok) {
    update_scene_label(g_eye_pair_active ? "render_fail_pair" : "render_fail");
  }
}

bool parse_style_name(const char *name, DeskRoboFaceStyle &out) {
  if (!name) return false;
  if ((strcmp(name, "EVE") == 0) || (strcmp(name, "eve") == 0) ||
      (strcmp(name, "EVE_CINEMATIC") == 0) || (strcmp(name, "eve_cinematic") == 0)) {
    out = DESKROBO_STYLE_EVE;
    return true;
  }
  return false;
}

const char *style_name(DeskRoboFaceStyle style) {
  (void)style;
  return "EVE_CINEMATIC";
}

void load_prefs_values() {
  if (!g_prefs.begin("deskrobo", true)) return;
  g_style = DESKROBO_STYLE_EVE;
  g_tuning.drift_amp_px = g_prefs.getInt("drift_amp_px", g_tuning.drift_amp_px);
  g_tuning.saccade_amp_px = g_prefs.getInt("saccade_amp_px", g_tuning.saccade_amp_px);
  g_tuning.saccade_min_ms = g_prefs.getInt("saccade_min_ms", g_tuning.saccade_min_ms);
  g_tuning.saccade_max_ms = g_prefs.getInt("saccade_max_ms", g_tuning.saccade_max_ms);
  g_tuning.blink_interval_ms = g_prefs.getInt("blink_interval", g_tuning.blink_interval_ms);
  g_tuning.blink_duration_ms = g_prefs.getInt("blink_duration", g_tuning.blink_duration_ms);
  g_tuning.double_blink_chance_pct = g_prefs.getInt("dbl_blink_pct", g_tuning.double_blink_chance_pct);
  g_tuning.glow_pulse_amp = g_prefs.getInt("glow_pulse_amp", g_tuning.glow_pulse_amp);
  g_tuning.glow_pulse_period_ms = g_prefs.getInt("glow_pulse_ms", g_tuning.glow_pulse_period_ms);
  g_tuning.shake_amp_px = g_prefs.getInt("shake_amp_px", g_tuning.shake_amp_px);
  g_tuning.shake_period_ms = g_prefs.getInt("shake_period_ms", g_tuning.shake_period_ms);
  g_tuning.sleep_delay_min = g_prefs.getInt("sleep_delay_min", g_tuning.sleep_delay_min);
  g_tuning.display_off_delay_min = g_prefs.getInt("display_off_delay_min", g_tuning.display_off_delay_min);
  g_tuning.eye_color_r = g_prefs.getInt("eye_color_r", g_tuning.eye_color_r);
  g_tuning.eye_color_g = g_prefs.getInt("eye_color_g", g_tuning.eye_color_g);
  g_tuning.eye_color_b = g_prefs.getInt("eye_color_b", g_tuning.eye_color_b);
  g_tuning.gyro_tilt_xy_pct = g_prefs.getInt("gyro_tilt_xy_pct", g_tuning.gyro_tilt_xy_pct);
  g_tuning.gyro_tilt_z_pct = g_prefs.getInt("gyro_tilt_z_pct", g_tuning.gyro_tilt_z_pct);
  g_tuning.gyro_tilt_cooldown_ms = g_prefs.getInt("gyro_tilt_cooldown", g_tuning.gyro_tilt_cooldown_ms);
  g_prefs.end();

  g_tuning.drift_amp_px = constrain(g_tuning.drift_amp_px, 1, 10);
  g_tuning.saccade_amp_px = constrain(g_tuning.saccade_amp_px, 0, 16);
  g_tuning.saccade_min_ms = constrain(g_tuning.saccade_min_ms, 250, 15000);
  g_tuning.saccade_max_ms = constrain(g_tuning.saccade_max_ms, g_tuning.saccade_min_ms, 20000);
  g_tuning.blink_interval_ms = constrain(g_tuning.blink_interval_ms, 600, 15000);
  g_tuning.blink_duration_ms = constrain(g_tuning.blink_duration_ms, 40, 1500);
  g_tuning.double_blink_chance_pct = constrain(g_tuning.double_blink_chance_pct, 0, 100);
  g_tuning.glow_pulse_amp = constrain(g_tuning.glow_pulse_amp, 0, 24);
  g_tuning.glow_pulse_period_ms = constrain(g_tuning.glow_pulse_period_ms, 400, 15000);
  g_tuning.shake_amp_px = constrain(g_tuning.shake_amp_px, 0, 80);
  g_tuning.shake_period_ms = constrain(g_tuning.shake_period_ms, 120, 4000);
  g_tuning.sleep_delay_min = constrain(g_tuning.sleep_delay_min, 1, 720);
  g_tuning.display_off_delay_min = constrain(g_tuning.display_off_delay_min, g_tuning.sleep_delay_min, 720);
  g_tuning.eye_color_r = constrain(g_tuning.eye_color_r, 0, 255);
  g_tuning.eye_color_g = constrain(g_tuning.eye_color_g, 0, 255);
  g_tuning.eye_color_b = constrain(g_tuning.eye_color_b, 0, 255);
  g_tuning.gyro_tilt_xy_pct = constrain(g_tuning.gyro_tilt_xy_pct, 0, 100);
  g_tuning.gyro_tilt_z_pct = constrain(g_tuning.gyro_tilt_z_pct, 0, 100);
  g_tuning.gyro_tilt_cooldown_ms = constrain(g_tuning.gyro_tilt_cooldown_ms, 100, 15000);
}
void apply_event_emotion(DeskRoboEmotion emotion, uint8_t priority, uint32_t ttl_ms) {
  const uint32_t now = millis();
  if ((priority < g_current_priority) && (now < g_emotion_expiry_ms)) return;
  g_eye_pair_active = false;
  g_current_emotion = emotion;
  g_current_priority = priority;
  g_emotion_expiry_ms = now + ttl_ms;
}

}  // namespace

void DeskRobo_Init() {
  randomSeed(micros());
  create_ui();
  load_prefs_values();
  // Start at last index so the first maybe_idle_round_robin call wraps,
  // shuffles, and begins at [0] — ensuring all positions are covered.
  g_idle_round_index = kIdleRoundLen - 1;
  g_last_interaction_ms = millis();
  g_gaze.next_gaze_ms = g_last_interaction_ms + 6000;
  g_micro_expr_next_ms = g_last_interaction_ms + 180000UL;
  g_boot_splash_started_ms = g_last_interaction_ms;
  reset_blink_state();
  reset_glitch_fx();
  update_status_label();
}

void DeskRobo_Loop() {
  maybe_expire_states();
  apply_pre_sleep_emotion();
  maybe_micro_expression();
  maybe_idle_round_robin();
  apply_sleep_policy();
  update_status_label();
  render_active();
  update_boot_splash();
}

void DeskRobo_PushEvent(DeskRoboEventType event_type) {
  mark_interaction(millis());
  switch (event_type) {
    case DESKROBO_EVENT_PC_CALL: apply_event_emotion(DESKROBO_EMOTION_CALL, 90, 5000); break;
    case DESKROBO_EVENT_PC_MAIL: apply_event_emotion(DESKROBO_EMOTION_CONFUSED, 70, 4000); break;
    case DESKROBO_EVENT_PC_TEAMS: apply_event_emotion(DESKROBO_EMOTION_WOW, 70, 4000); break;
    case DESKROBO_EVENT_AUDIO_LOUD: apply_event_emotion(DESKROBO_EMOTION_WOW, 60, 2500); break;
    case DESKROBO_EVENT_AUDIO_VERY_LOUD: apply_event_emotion(DESKROBO_EMOTION_SHAKE, 85, 2600); break;
    case DESKROBO_EVENT_MOTION_TILT: apply_event_emotion(DESKROBO_EMOTION_CONFUSED, 55, 2200); break;
    case DESKROBO_EVENT_MOTION_SHAKE:
    case DESKROBO_EVENT_MOTION_TAP: apply_event_emotion(DESKROBO_EMOTION_SHAKE, 85, 2600); break;
    case DESKROBO_EVENT_AUDIO_QUIET:
    case DESKROBO_EVENT_NONE:
    default: apply_event_emotion(DESKROBO_EMOTION_IDLE, 1, 1000); break;
  }
}

DeskRoboEmotion DeskRobo_GetEmotion() { return g_current_emotion; }

void DeskRobo_SetEmotion(DeskRoboEmotion emotion, uint32_t hold_ms) {
  const uint32_t now = millis();
  mark_interaction(now);
  g_eye_pair_active = false;
  g_current_emotion = emotion;
  g_current_priority = 100;
  g_emotion_expiry_ms = now + hold_ms;
}

void DeskRobo_SetEyePair(DeskRoboEmotion left, DeskRoboEmotion right, uint32_t hold_ms) {
  const uint32_t now = millis();
  mark_interaction(now);
  g_left_eye_emotion = left;
  g_right_eye_emotion = right;
  g_eye_pair_active = true;
  g_eye_pair_expiry_ms = now + hold_ms;
  g_current_priority = 100;
  g_emotion_expiry_ms = g_eye_pair_expiry_ms;
}

bool DeskRobo_SetTuning(const char *key, int value) {
  if (!key) return false;
  if (strcmp(key, "drift_amp_px") == 0) g_tuning.drift_amp_px = constrain(value, 0, 12);
  else if (strcmp(key, "saccade_amp_px") == 0) g_tuning.saccade_amp_px = constrain(value, 0, 18);
  else if (strcmp(key, "saccade_min_ms") == 0) g_tuning.saccade_min_ms = constrain(value, 200, 20000);
  else if (strcmp(key, "saccade_max_ms") == 0) g_tuning.saccade_max_ms = constrain(value, 200, 25000);
  else if (strcmp(key, "blink_interval_ms") == 0) g_tuning.blink_interval_ms = constrain(value, 1000, 20000);
  else if (strcmp(key, "blink_duration_ms") == 0) g_tuning.blink_duration_ms = constrain(value, 40, 1000);
  else if (strcmp(key, "double_blink_chance_pct") == 0) g_tuning.double_blink_chance_pct = constrain(value, 0, 100);
  else if (strcmp(key, "glow_pulse_amp") == 0) g_tuning.glow_pulse_amp = constrain(value, 0, 24);
  else if (strcmp(key, "glow_pulse_period_ms") == 0) g_tuning.glow_pulse_period_ms = constrain(value, 200, 10000);
  else if (strcmp(key, "shake_amp_px") == 0) g_tuning.shake_amp_px = constrain(value, 0, 60);
  else if (strcmp(key, "shake_period_ms") == 0) g_tuning.shake_period_ms = constrain(value, 120, 4000);
  else if (strcmp(key, "sleep_delay_min") == 0) g_tuning.sleep_delay_min = constrain(value, 1, 240);
  else if (strcmp(key, "display_off_delay_min") == 0) g_tuning.display_off_delay_min = constrain(value, 1, 480);
  else if (strcmp(key, "eye_color_r") == 0) g_tuning.eye_color_r = constrain(value, 0, 255);
  else if (strcmp(key, "eye_color_g") == 0) g_tuning.eye_color_g = constrain(value, 0, 255);
  else if (strcmp(key, "eye_color_b") == 0) g_tuning.eye_color_b = constrain(value, 0, 255);
  else if (strcmp(key, "gyro_tilt_xy_pct") == 0) g_tuning.gyro_tilt_xy_pct = constrain(value, 40, 90);
  else if (strcmp(key, "gyro_tilt_z_pct") == 0) g_tuning.gyro_tilt_z_pct = constrain(value, 30, 90);
  else if (strcmp(key, "gyro_tilt_cooldown_ms") == 0) g_tuning.gyro_tilt_cooldown_ms = constrain(value, 400, 10000);
  else return false;
  if (g_tuning.display_off_delay_min < g_tuning.sleep_delay_min) g_tuning.display_off_delay_min = g_tuning.sleep_delay_min;
  return true;
}

String DeskRobo_GetTuningJson() {
  String out = "{";
  out += "\"drift_amp_px\":" + String(g_tuning.drift_amp_px);
  out += ",\"saccade_amp_px\":" + String(g_tuning.saccade_amp_px);
  out += ",\"saccade_min_ms\":" + String(g_tuning.saccade_min_ms);
  out += ",\"saccade_max_ms\":" + String(g_tuning.saccade_max_ms);
  out += ",\"blink_interval_ms\":" + String(g_tuning.blink_interval_ms);
  out += ",\"blink_duration_ms\":" + String(g_tuning.blink_duration_ms);
  out += ",\"double_blink_chance_pct\":" + String(g_tuning.double_blink_chance_pct);
  out += ",\"glow_pulse_amp\":" + String(g_tuning.glow_pulse_amp);
  out += ",\"glow_pulse_period_ms\":" + String(g_tuning.glow_pulse_period_ms);
  out += ",\"shake_amp_px\":" + String(g_tuning.shake_amp_px);
  out += ",\"shake_period_ms\":" + String(g_tuning.shake_period_ms);
  out += ",\"sleep_delay_min\":" + String(g_tuning.sleep_delay_min);
  out += ",\"display_off_delay_min\":" + String(g_tuning.display_off_delay_min);
  out += ",\"eye_color_r\":" + String(g_tuning.eye_color_r);
  out += ",\"eye_color_g\":" + String(g_tuning.eye_color_g);
  out += ",\"eye_color_b\":" + String(g_tuning.eye_color_b);
  out += ",\"gyro_tilt_xy_pct\":" + String(g_tuning.gyro_tilt_xy_pct);
  out += ",\"gyro_tilt_z_pct\":" + String(g_tuning.gyro_tilt_z_pct);
  out += ",\"gyro_tilt_cooldown_ms\":" + String(g_tuning.gyro_tilt_cooldown_ms);
  out += "}";
  return out;
}

bool DeskRobo_SaveTuning() {
  if (!g_prefs.begin("deskrobo", false)) return false;
  g_prefs.putInt("face_style", (int)g_style);
  g_prefs.putInt("drift_amp_px", g_tuning.drift_amp_px);
  g_prefs.putInt("saccade_amp_px", g_tuning.saccade_amp_px);
  g_prefs.putInt("saccade_min_ms", g_tuning.saccade_min_ms);
  g_prefs.putInt("saccade_max_ms", g_tuning.saccade_max_ms);
  g_prefs.putInt("blink_interval", g_tuning.blink_interval_ms);
  g_prefs.putInt("blink_duration", g_tuning.blink_duration_ms);
  g_prefs.putInt("dbl_blink_pct", g_tuning.double_blink_chance_pct);
  g_prefs.putInt("glow_pulse_amp", g_tuning.glow_pulse_amp);
  g_prefs.putInt("glow_pulse_ms", g_tuning.glow_pulse_period_ms);
  g_prefs.putInt("shake_amp_px", g_tuning.shake_amp_px);
  g_prefs.putInt("shake_period_ms", g_tuning.shake_period_ms);
  g_prefs.putInt("sleep_delay_min", g_tuning.sleep_delay_min);
  g_prefs.putInt("display_off_delay_min", g_tuning.display_off_delay_min);
  g_prefs.putInt("eye_color_r", g_tuning.eye_color_r);
  g_prefs.putInt("eye_color_g", g_tuning.eye_color_g);
  g_prefs.putInt("eye_color_b", g_tuning.eye_color_b);
  g_prefs.putInt("gyro_tilt_xy_pct", g_tuning.gyro_tilt_xy_pct);
  g_prefs.putInt("gyro_tilt_z_pct", g_tuning.gyro_tilt_z_pct);
  g_prefs.putInt("gyro_tilt_cooldown", g_tuning.gyro_tilt_cooldown_ms);
  g_prefs.end();
  return true;
}

bool DeskRobo_LoadTuning() {
  load_prefs_values();
  return true;
}

bool DeskRobo_SetStyleByName(const char *name) {
  DeskRoboFaceStyle new_style;
  if (!parse_style_name(name, new_style)) return false;
  g_style = DESKROBO_STYLE_EVE;
  return true;
}

const char *DeskRobo_GetStyleName() { return style_name(g_style); }

void DeskRobo_SetBacklightLevel(uint8_t value) {
  const uint8_t v = (uint8_t)constrain((int)value, 0, 100);
  g_backlight_before_sleep = v > 0 ? v : g_backlight_before_sleep;
  g_display_sleep_off = false;
  g_display_dimmed = false;
  Set_Backlight(v);
  g_last_interaction_ms = millis();
}

void DeskRobo_SetStatusLabelVisible(bool visible) {
  g_status_label_visible = visible;
  update_status_label();
}

bool DeskRobo_GetStatusLabelVisible() { return g_status_label_visible; }

void DeskRobo_SetBleConnected(bool connected) {
  g_ble_connected = connected;
  if (connected) mark_interaction(millis());
}




















