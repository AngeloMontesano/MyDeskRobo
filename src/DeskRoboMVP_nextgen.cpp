#include "DeskRoboMVP.h"

#include <Preferences.h>
#include <WiFi.h>
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

using namespace nse;

namespace {

struct NextgenTuning {
  int drift_amp_px = 2;
  int saccade_amp_px = 5;
  int saccade_min_ms = 1400;
  int saccade_max_ms = 3800;
  int blink_interval_ms = 3600;
  int blink_duration_ms = 120;
  int double_blink_chance_pct = 20;
  int glow_pulse_amp = 6;
  int glow_pulse_period_ms = 2600;
  int shake_amp_px = 24;
  int shake_period_ms = 700;
  int sleep_delay_min = 15;
  int display_off_delay_min = 30;
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

LayerRenderer *g_renderer = nullptr;
LayerRenderer *g_glitch_red_renderer = nullptr;
LayerRenderer *g_glitch_cyan_renderer = nullptr;
lv_obj_t *g_canvas = nullptr;
lv_obj_t *g_glitch_red_canvas = nullptr;
lv_obj_t *g_glitch_cyan_canvas = nullptr;
lv_obj_t *g_status_label = nullptr;
lv_obj_t *g_scene_label = nullptr;
lv_obj_t *g_sleep_labels[3] = {nullptr, nullptr, nullptr};
lv_obj_t *g_confused_label = nullptr;

Preferences g_prefs;
NextgenTuning g_tuning;
GlitchFxState g_glitch_fx;

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

static constexpr uint32_t kIdleRoundRobinAfterMs = 60UL * 1000UL;
static constexpr uint32_t kIdleRoundRobinIntervalMs = 45UL * 1000UL;
static constexpr uint32_t kIdleRoundRobinShowMs = 6000UL;
static constexpr uint8_t kIdleRoundRobinPriority = 5;
static constexpr uint8_t kScreensaverDimPct = 35;
static constexpr DeskRoboEmotion kIdleRound[] = {
    DESKROBO_EMOTION_IDLE,
    DESKROBO_EMOTION_HAPPY,
    DESKROBO_EMOTION_IDLE,
    DESKROBO_EMOTION_WOW,
    DESKROBO_EMOTION_IDLE,
    DESKROBO_EMOTION_CONFUSED,
};

const char *emotion_name(DeskRoboEmotion emotion) {
  switch (emotion) {
    case DESKROBO_EMOTION_HAPPY: return "HAPPY";
    case DESKROBO_EMOTION_SAD: return "SAD";
    case DESKROBO_EMOTION_ANGRY: return "ANGRY";
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
    default: return "IDLE";
  }
}

lv_color_t eye_color() {
  return lv_color_make((uint8_t)constrain(g_tuning.eye_color_r, 0, 255),
                       (uint8_t)constrain(g_tuning.eye_color_g, 0, 255),
                       (uint8_t)constrain(g_tuning.eye_color_b, 0, 255));
}

bool wifi_session_active() {
  if (WiFi.status() == WL_CONNECTED) return true;
  return WiFi.softAPgetStationNum() > 0;
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
  lv_label_set_text_fmt(g_scene_label, "%s", scene_name ? scene_name : "scene");
}

void reset_blink_state() {
  g_last_blink_toggle_ms = millis();
  g_blink_active = false;
  g_double_blink_pending = false;
}

void reset_glitch_fx() {
  g_glitch_fx = GlitchFxState{};
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
  if (!g_blink_active && (now - g_last_blink_toggle_ms) > (uint32_t)g_tuning.blink_interval_ms) {
    g_blink_active = true;
    g_last_blink_toggle_ms = now;
  } else if (g_blink_active && (now - g_last_blink_toggle_ms) > (uint32_t)g_tuning.blink_duration_ms) {
    g_blink_active = false;
    g_last_blink_toggle_ms = now;
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
  uint16_t saccade_x_period = (uint16_t)g_tuning.saccade_min_ms;
  uint16_t saccade_y_period = (uint16_t)g_tuning.saccade_max_ms;
  if (strcmp(scene.name, "eve_shake") == 0) {
    drift_amp = g_tuning.shake_amp_px;
    saccade_amp = g_tuning.shake_amp_px / 2;
    drift_x_period = (uint16_t)g_tuning.shake_period_ms;
    drift_y_period = (uint16_t)max(120, g_tuning.shake_period_ms + 100);
    saccade_x_period = (uint16_t)max(120, g_tuning.shake_period_ms / 2);
    saccade_y_period = (uint16_t)max(180, g_tuning.shake_period_ms);
  }
  state.drift_x = wave_value(now, drift_x_period, drift_amp, false);
  state.drift_y = wave_value(now, drift_y_period, drift_amp / 2, true);
  state.saccade_x = wave_value(now, saccade_x_period, saccade_amp, false);
  state.saccade_y = wave_value(now, saccade_y_period, saccade_amp / 2, true);
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
    if (g_glitch_cyan_canvas) lv_obj_add_flag(g_glitch_cyan_canvas, LV_OBJ_FLAG_HIDDEN);
    if (g_glitch_red_canvas) lv_obj_add_flag(g_glitch_red_canvas, LV_OBJ_FLAG_HIDDEN);
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
      g_glitch_fx.scan_h[i] = (int16_t)random(1, 3);
    }
  }

  if (g_glitch_fx.split_frames > 0) {
    lv_obj_clear_flag(g_glitch_cyan_canvas, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(g_glitch_red_canvas, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(g_glitch_cyan_canvas, -4, 0);
    lv_obj_set_pos(g_glitch_red_canvas, 2, 0);
    lv_obj_set_style_opa(g_glitch_cyan_canvas, LV_OPA_70, 0);
    lv_obj_set_style_opa(g_glitch_red_canvas, LV_OPA_90, 0);
  } else {
    lv_obj_add_flag(g_glitch_cyan_canvas, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(g_glitch_red_canvas, LV_OBJ_FLAG_HIDDEN);
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

void maybe_idle_round_robin() {
  const uint32_t now = millis();
  if (g_eye_pair_active || g_current_priority > 0) return;
  if ((now - g_last_interaction_ms) < kIdleRoundRobinAfterMs) return;
  if ((now - g_last_idle_round_ms) < kIdleRoundRobinIntervalMs) return;
  g_last_idle_round_ms = now;
  g_idle_round_index = (uint8_t)((g_idle_round_index + 1) % (sizeof(kIdleRound) / sizeof(kIdleRound[0])));
  g_current_emotion = kIdleRound[g_idle_round_index];
  g_current_priority = kIdleRoundRobinPriority;
  g_emotion_expiry_ms = now + kIdleRoundRobinShowMs;
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

void init_canvas(lv_obj_t *canvas) {
  lv_obj_remove_style_all(canvas);
  lv_obj_clear_flag(canvas, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(canvas, EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT);
  lv_obj_set_pos(canvas, 0, 0);
  lv_obj_set_style_bg_color(canvas, lv_color_hex(0x06080D), 0);
  lv_obj_set_style_bg_opa(canvas, LV_OPA_COVER, 0);
}

void create_ui() {
  g_glitch_cyan_canvas = lv_canvas_create(lv_scr_act());
  init_canvas(g_glitch_cyan_canvas);
  lv_obj_add_flag(g_glitch_cyan_canvas, LV_OBJ_FLAG_HIDDEN);

  g_glitch_red_canvas = lv_canvas_create(lv_scr_act());
  init_canvas(g_glitch_red_canvas);
  lv_obj_add_flag(g_glitch_red_canvas, LV_OBJ_FLAG_HIDDEN);

  g_canvas = lv_canvas_create(lv_scr_act());
  init_canvas(g_canvas);

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
  lv_obj_set_style_text_font(g_confused_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_transform_zoom(g_confused_label, 360, 0);
  lv_obj_set_style_bg_opa(g_confused_label, LV_OPA_TRANSP, 0);
  lv_obj_add_flag(g_confused_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_pos(g_confused_label, 250, 28);

  static LayerRenderer renderer(g_canvas);
  static LayerRenderer glitch_red_renderer(g_glitch_red_canvas);
  static LayerRenderer glitch_cyan_renderer(g_glitch_cyan_canvas);
  g_renderer = &renderer;
  g_glitch_red_renderer = &glitch_red_renderer;
  g_glitch_cyan_renderer = &glitch_cyan_renderer;
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

  lv_color_t base_eye_color = eye_color();
  if (g_eye_pair_active) {
    RuntimeState left_state = make_runtime_state(left_scene, base_eye_color, is_glitch_scene(left_scene));
    RuntimeState right_state = make_runtime_state(right_scene, base_eye_color, is_glitch_scene(right_scene));
    g_renderer->render_pair(left_scene, left_state, right_scene, right_state);
    return;
  }

  if (is_glitch_scene(left_scene)) {
    const bool glitch_active = (g_glitch_fx.row_frames > 0) || (g_glitch_fx.flicker_frames > 0) || (g_glitch_fx.split_frames > 0) || (g_glitch_fx.scan_frames > 0);
    const RuntimeState main_state = make_runtime_state(left_scene, glitch_active ? lv_color_make(0xFF, 0x00, 0x40) : base_eye_color, true);
    const RuntimeState cyan_state = make_runtime_state(left_scene, lv_color_make(0x00, 0xFF, 0xFF), true);
    const RuntimeState red_state = make_runtime_state(left_scene, lv_color_make(0xFF, 0x00, 0x40), true);
    g_renderer->render(left_scene, main_state);
    if (g_glitch_fx.split_frames > 0) {
      lv_obj_clear_flag(g_glitch_cyan_canvas, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(g_glitch_red_canvas, LV_OBJ_FLAG_HIDDEN);
      g_glitch_cyan_renderer->render(left_scene, cyan_state);
      g_glitch_red_renderer->render(left_scene, red_state);
    }
    return;
  }

  RuntimeState state = make_runtime_state(left_scene, base_eye_color, false);
  g_renderer->render(left_scene, state);
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
  g_last_interaction_ms = millis();
  reset_blink_state();
  reset_glitch_fx();
  update_status_label();
}

void DeskRobo_Loop() {
  maybe_expire_states();
  maybe_idle_round_robin();
  apply_sleep_policy();
  update_status_label();
  render_active();
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

void DeskRobo_SetStatusLabelVisible(bool visible) {
  g_status_label_visible = visible;
  update_status_label();
}

bool DeskRobo_GetStatusLabelVisible() { return g_status_label_visible; }

void DeskRobo_SetBleConnected(bool connected) {
  g_ble_connected = connected;
  if (connected) mark_interaction(millis());
}





