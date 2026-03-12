#include "DeskRoboMVP.h"

#include <cstring>
#include <lvgl.h>
#include <math.h>
#include <Preferences.h>
#include <WiFi.h>

#include "LVGL_Arduino/Display_ST77916.h"
#include "LVGL_Arduino/Gyro_QMI8658.h"

#include "anime_face.h"

#ifndef DESKROBO_ENABLE_GYRO
#define DESKROBO_ENABLE_GYRO 0
#endif

#ifndef DESKROBO_GYRO_EVENTS
#define DESKROBO_GYRO_EVENTS 1
#endif

#ifndef DESKROBO_SHOW_DEBUG
#define DESKROBO_SHOW_DEBUG 0
#endif

// Set to the analog mic pin if available on your board. -1 disables local mic.
#ifndef DESKROBO_AUDIO_ADC_PIN
#define DESKROBO_AUDIO_ADC_PIN (-1)
#endif

#ifndef DESKROBO_BLE_DISCONNECT_SLEEP_OFF_MS
#define DESKROBO_BLE_DISCONNECT_SLEEP_OFF_MS (15UL * 60UL * 1000UL)
#endif

#ifndef DESKROBO_DEFAULT_WAKE_BACKLIGHT
#define DESKROBO_DEFAULT_WAKE_BACKLIGHT 15
#endif

#ifndef DESKROBO_BLE_SCREENSAVER_DELAY_MS
#define DESKROBO_BLE_SCREENSAVER_DELAY_MS (15UL * 60UL * 1000UL)
#endif

#ifndef DESKROBO_BLE_DISPLAY_OFF_DELAY_MS
#define DESKROBO_BLE_DISPLAY_OFF_DELAY_MS (30UL * 60UL * 1000UL)
#endif

#ifndef DESKROBO_BLE_SCREENSAVER_DIM_PCT
#define DESKROBO_BLE_SCREENSAVER_DIM_PCT 35
#endif

#ifndef DESKROBO_IDLE_ROUND_ROBIN_AFTER_MS
#define DESKROBO_IDLE_ROUND_ROBIN_AFTER_MS (4UL * 60UL * 1000UL)
#endif

#ifndef DESKROBO_IDLE_ROUND_ROBIN_INTERVAL_MS
#define DESKROBO_IDLE_ROUND_ROBIN_INTERVAL_MS (45UL * 1000UL)
#endif

#ifndef DESKROBO_IDLE_ROUND_ROBIN_SHOW_MS
#define DESKROBO_IDLE_ROUND_ROBIN_SHOW_MS (5000UL)
#endif

typedef struct {
  DeskRoboEmotion emotion;
  uint8_t priority;
  uint32_t ttl_ms;
} DeskRoboEventRule;

static AnimeFace s_face;
static lv_obj_t *s_status = nullptr;
static lv_obj_t *s_motion_debug = nullptr;
static lv_obj_t *s_time_label = nullptr;
static bool s_status_label_visible = false;

static DeskRoboEmotion s_current_emotion = DESKROBO_EMOTION_IDLE;
static uint8_t s_current_priority = 0;
static uint32_t s_emotion_expiry_ms = 0;
static bool s_eye_pair_active = false;
static DeskRoboEmotion s_left_eye_emotion = DESKROBO_EMOTION_IDLE;
static DeskRoboEmotion s_right_eye_emotion = DESKROBO_EMOTION_IDLE;
static uint32_t s_eye_pair_expiry_ms = 0;
static uint32_t s_time_expiry_ms = 0;

static bool s_ble_connected = false;
static bool s_ble_state_seen = false;
static bool s_ble_sleep_face_active = false;
static bool s_display_sleep_off = false;
static bool s_display_dimmed = false;
static uint32_t s_ble_disconnected_since_ms = 0;
static uint8_t s_backlight_before_sleep = DESKROBO_DEFAULT_WAKE_BACKLIGHT;
static uint32_t s_last_interaction_ms = 0;
static uint32_t s_last_idle_round_ms = 0;
static uint8_t s_idle_round_index = 0;

static uint32_t s_last_sensor_ms = 0;
static uint32_t s_last_motion_ms = 0;
static uint32_t s_last_tilt_event_ms = 0;
static uint32_t s_last_shake_event_ms = 0;
static const char *s_last_motion_event = "none";

static int32_t s_audio_baseline = 0;
static int32_t s_audio_level = 0;
static Preferences s_prefs;
static DeskRoboFaceStyle s_face_style = DESKROBO_STYLE_EVE;

static FaceStyle to_face_style(DeskRoboFaceStyle style) {
  if (style == DESKROBO_STYLE_ROUND) return FACE_STYLE_ROUND;
  if (style == DESKROBO_STYLE_EVE_SUBTLE) return FACE_STYLE_EVE_SUBTLE;
  if (style == DESKROBO_STYLE_EVE_COMIC) return FACE_STYLE_EVE_COMIC;
  return FACE_STYLE_EVE_CINEMATIC;
}

static const char *style_name(DeskRoboFaceStyle style) {
  if (style == DESKROBO_STYLE_ROUND) return "ROUND";
  if (style == DESKROBO_STYLE_EVE_SUBTLE) return "EVE_SUBTLE";
  if (style == DESKROBO_STYLE_EVE_COMIC) return "EVE_COMIC";
  return "EVE_CINEMATIC";
}

static bool parse_style_name(const char *name, DeskRoboFaceStyle &out) {
  if (!name) return false;
  if ((strcmp(name, "EVE") == 0) || (strcmp(name, "eve") == 0) ||
      (strcmp(name, "EVE_CINEMATIC") == 0) ||
      (strcmp(name, "eve_cinematic") == 0)) {
    out = DESKROBO_STYLE_EVE;
    return true;
  }
  if ((strcmp(name, "EVE_SUBTLE") == 0) ||
      (strcmp(name, "eve_subtle") == 0)) {
    out = DESKROBO_STYLE_EVE_SUBTLE;
    return true;
  }
  if ((strcmp(name, "EVE_COMIC") == 0) || (strcmp(name, "eve_comic") == 0)) {
    out = DESKROBO_STYLE_EVE_COMIC;
    return true;
  }
  if ((strcmp(name, "ROUND") == 0) || (strcmp(name, "round") == 0)) {
    out = DESKROBO_STYLE_ROUND;
    return true;
  }
  return false;
}

static bool parse_emotion_name(const char *name, DeskRoboEmotion &out) {
  if (!name) return false;
  if (strcmp(name, "HAPPY") == 0) {
    out = DESKROBO_EMOTION_HAPPY;
    return true;
  }
  if (strcmp(name, "SAD") == 0) {
    out = DESKROBO_EMOTION_SAD;
    return true;
  }
  if (strcmp(name, "ANGRY") == 0) {
    out = DESKROBO_EMOTION_ANGRY;
    return true;
  }
  if (strcmp(name, "WOW") == 0) {
    out = DESKROBO_EMOTION_WOW;
    return true;
  }
  if (strcmp(name, "SLEEPY") == 0) {
    out = DESKROBO_EMOTION_SLEEPY;
    return true;
  }
  if (strcmp(name, "CONFUSED") == 0) {
    out = DESKROBO_EMOTION_CONFUSED;
    return true;
  }
  if (strcmp(name, "EXCITED") == 0) {
    out = DESKROBO_EMOTION_EXCITED;
    return true;
  }
  if (strcmp(name, "DIZZY") == 0) {
    out = DESKROBO_EMOTION_DIZZY;
    return true;
  }
  if (strcmp(name, "MAIL") == 0) {
    out = DESKROBO_EMOTION_MAIL;
    return true;
  }
  if (strcmp(name, "CALL") == 0) {
    out = DESKROBO_EMOTION_CALL;
    return true;
  }
  if (strcmp(name, "SHAKE") == 0) {
    out = DESKROBO_EMOTION_SHAKE;
    return true;
  }
  if (strcmp(name, "IDLE") == 0) {
    out = DESKROBO_EMOTION_IDLE;
    return true;
  }
  return false;
}

static Emotion to_anime(DeskRoboEmotion emotion) {
  switch (emotion) {
    case DESKROBO_EMOTION_HAPPY:
      return EMOTION_HAPPY;
    case DESKROBO_EMOTION_SAD:
      return EMOTION_SAD;
    case DESKROBO_EMOTION_ANGRY:
      return EMOTION_ANGRY;
    case DESKROBO_EMOTION_WOW:
      return EMOTION_WOW;
    case DESKROBO_EMOTION_SLEEPY:
      return EMOTION_SLEEPY;
    case DESKROBO_EMOTION_CONFUSED:
      return EMOTION_CONFUSED;
    case DESKROBO_EMOTION_EXCITED:
      return EMOTION_EXCITED;
    case DESKROBO_EMOTION_DIZZY:
      return EMOTION_DIZZY;
    case DESKROBO_EMOTION_MAIL:
      return EMOTION_MAIL;
    case DESKROBO_EMOTION_CALL:
      return EMOTION_CALL;
    case DESKROBO_EMOTION_SHAKE:
      return EMOTION_SHAKE;
    case DESKROBO_EMOTION_IDLE:
    default:
      return EMOTION_IDLE;
  }
}

static const char *emotion_name(DeskRoboEmotion emotion) {
  switch (emotion) {
    case DESKROBO_EMOTION_HAPPY:
      return "HAPPY";
    case DESKROBO_EMOTION_SAD:
      return "SAD";
    case DESKROBO_EMOTION_ANGRY:
      return "ANGRY";
    case DESKROBO_EMOTION_WOW:
      return "WOW";
    case DESKROBO_EMOTION_SLEEPY:
      return "SLEEPY";
    case DESKROBO_EMOTION_CONFUSED:
      return "CONFUSED";
    case DESKROBO_EMOTION_EXCITED:
      return "EXCITED";
    case DESKROBO_EMOTION_DIZZY:
      return "DIZZY";
    case DESKROBO_EMOTION_MAIL:
      return "MAIL";
    case DESKROBO_EMOTION_CALL:
      return "CALL";
    case DESKROBO_EMOTION_SHAKE:
      return "SHAKE";
    case DESKROBO_EMOTION_IDLE:
    default:
      return "IDLE";
  }
}

static DeskRoboEventRule event_rule(DeskRoboEventType event_type) {
  switch (event_type) {
    case DESKROBO_EVENT_PC_CALL:
      return {DESKROBO_EMOTION_CALL, 90, 8000};
    case DESKROBO_EVENT_PC_TEAMS:
      return {DESKROBO_EMOTION_CONFUSED, 80, 5000};
    case DESKROBO_EVENT_PC_MAIL:
      return {DESKROBO_EMOTION_MAIL, 70, 3500};
    case DESKROBO_EVENT_MOTION_SHAKE:
      return {DESKROBO_EMOTION_SHAKE, 60, 2500};
    case DESKROBO_EVENT_AUDIO_VERY_LOUD:
      return {DESKROBO_EMOTION_WOW, 50, 1800};
    case DESKROBO_EVENT_AUDIO_LOUD:
      return {DESKROBO_EMOTION_ANGRY, 40, 1400};
    case DESKROBO_EVENT_MOTION_TILT:
      return {DESKROBO_EMOTION_CONFUSED, 30, 1200};
    case DESKROBO_EVENT_AUDIO_QUIET:
      return {DESKROBO_EMOTION_SLEEPY, 10, 1200};
    case DESKROBO_EVENT_NONE:
    default:
      return {DESKROBO_EMOTION_IDLE, 0, 0};
  }
}

static void apply_emotion_style() {
  s_face.setStyle(to_face_style(s_face_style));
  if (s_eye_pair_active) {
    s_face.setEyePair(to_anime(s_left_eye_emotion), to_anime(s_right_eye_emotion));
  } else {
    s_face.clearEyePair();
    s_face.setEmotion(to_anime(s_current_emotion));
  }
  if (s_status) {
    lv_label_set_text_fmt(s_status, "DeskRobo: %s", emotion_name(s_current_emotion));
    if (s_status_label_visible) {
      lv_obj_clear_flag(s_status, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(s_status, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

static uint8_t wake_backlight_level() {
  if (s_backlight_before_sleep > 0) return s_backlight_before_sleep;
  if (LCD_Backlight > 0) return LCD_Backlight;
  return DESKROBO_DEFAULT_WAKE_BACKLIGHT;
}

static void wake_display_from_sleep() {
  if (!s_display_sleep_off && !s_display_dimmed) return;
  Set_Backlight(wake_backlight_level());
  s_display_sleep_off = false;
  s_display_dimmed = false;
}

static bool wifi_session_active() {
  if (WiFi.status() == WL_CONNECTED) return true;
  return WiFi.softAPgetStationNum() > 0;
}

static void mark_interaction(uint32_t now) {
  s_last_interaction_ms = now;
  wake_display_from_sleep();

  if (s_ble_sleep_face_active) {
    s_ble_sleep_face_active = false;
    s_eye_pair_active = false;
    s_current_emotion = DESKROBO_EMOTION_IDLE;
    s_current_priority = 0;
    s_emotion_expiry_ms = 0;
    apply_emotion_style();
  }
}

static void apply_ble_disconnect_policy(uint32_t now) {
  if (s_ble_connected) return;

  if (s_ble_disconnected_since_ms == 0) {
    s_ble_disconnected_since_ms = now;
  }

  // In WLAN/AP sessions, do not force sleepy screensaver.
  if (wifi_session_active()) {
    if (s_ble_sleep_face_active) {
      s_ble_sleep_face_active = false;
      s_current_emotion = DESKROBO_EMOTION_IDLE;
      s_current_priority = 0;
      s_emotion_expiry_ms = 0;
      apply_emotion_style();
    }
    wake_display_from_sleep();
    return;
  }

  const uint32_t idle_ms = now - s_last_interaction_ms;
  if (idle_ms < DESKROBO_BLE_SCREENSAVER_DELAY_MS) {
    return;
  }

  if (!s_ble_sleep_face_active) {
    s_eye_pair_active = false;
    s_current_emotion = DESKROBO_EMOTION_SLEEPY;
    s_current_priority = 0;
    s_emotion_expiry_ms = 0;
    s_ble_sleep_face_active = true;
    apply_emotion_style();
    Serial.println("[MVP] Offline screensaver active");
  }

  if ((idle_ms >= DESKROBO_BLE_DISPLAY_OFF_DELAY_MS)) {
    if (!s_display_sleep_off) {
      if (LCD_Backlight > 0) s_backlight_before_sleep = LCD_Backlight;
      Set_Backlight(0);
      s_display_sleep_off = true;
      s_display_dimmed = false;
      Serial.println("[MVP] Offline timeout reached -> display off");
    }
    return;
  }

  // Between screensaver start and display-off timeout: keep dimmed sleepy view.
  if (!s_display_dimmed && !s_display_sleep_off) {
    if (LCD_Backlight > 0) s_backlight_before_sleep = LCD_Backlight;
    uint8_t dim_level =
        (uint8_t)((wake_backlight_level() * DESKROBO_BLE_SCREENSAVER_DIM_PCT) / 100U);
    if (dim_level < 2) dim_level = 2;
    Set_Backlight(dim_level);
    s_display_dimmed = true;
  }
}

void DeskRobo_SetEmotion(DeskRoboEmotion emotion, uint32_t hold_ms) {
  const uint32_t now = millis();
  mark_interaction(now);
  s_eye_pair_active = false;
  s_current_emotion = emotion;
  s_current_priority = 100;
  s_emotion_expiry_ms = now + hold_ms;
  apply_emotion_style();
}

void DeskRobo_SetEyePair(DeskRoboEmotion left, DeskRoboEmotion right,
                         uint32_t hold_ms) {
  const uint32_t now = millis();
  mark_interaction(now);
  s_left_eye_emotion = left;
  s_right_eye_emotion = right;
  s_eye_pair_active = true;
  s_eye_pair_expiry_ms = now + hold_ms;
  s_current_priority = 100;
  s_emotion_expiry_ms = s_eye_pair_expiry_ms;
  apply_emotion_style();
}

bool DeskRobo_SetTuning(const char *key, int value) {
  const bool ok = s_face.setTuningValue(key, value);
  if (ok) {
    apply_emotion_style();
  }
  return ok;
}

String DeskRobo_GetTuningJson() { return s_face.getTuningJson(); }

bool DeskRobo_SaveTuning() {
  if (!s_prefs.begin("deskrobo", false)) return false;
  s_prefs.putInt("face_style", (int)s_face_style);
  s_prefs.putInt("drift_amp_px", s_face.getTuningValue("drift_amp_px"));
  s_prefs.putInt("saccade_amp_px", s_face.getTuningValue("saccade_amp_px"));
  s_prefs.putInt("saccade_min_ms", s_face.getTuningValue("saccade_min_ms"));
  s_prefs.putInt("saccade_max_ms", s_face.getTuningValue("saccade_max_ms"));
  s_prefs.putInt("blink_interval", s_face.getTuningValue("blink_interval_ms"));
  s_prefs.putInt("blink_duration", s_face.getTuningValue("blink_duration_ms"));
  s_prefs.putInt("dbl_blink_pct",
                 s_face.getTuningValue("double_blink_chance_pct"));
  s_prefs.putInt("glow_pulse_amp", s_face.getTuningValue("glow_pulse_amp"));
  s_prefs.putInt("glow_pulse_ms", s_face.getTuningValue("glow_pulse_period_ms"));
  s_prefs.end();
  return true;
}

bool DeskRobo_LoadTuning() {
  if (!s_prefs.begin("deskrobo", true)) return false;
  const int face_style = s_prefs.getInt("face_style", (int)DESKROBO_STYLE_EVE);
  const int drift_amp_px =
      s_prefs.getInt("drift_amp_px", s_face.getTuningValue("drift_amp_px"));
  const int saccade_amp_px = s_prefs.getInt(
      "saccade_amp_px", s_face.getTuningValue("saccade_amp_px"));
  const int saccade_min_ms = s_prefs.getInt(
      "saccade_min_ms", s_face.getTuningValue("saccade_min_ms"));
  const int saccade_max_ms = s_prefs.getInt(
      "saccade_max_ms", s_face.getTuningValue("saccade_max_ms"));
  const int blink_interval_ms = s_prefs.getInt(
      "blink_interval", s_face.getTuningValue("blink_interval_ms"));
  const int blink_duration_ms = s_prefs.getInt(
      "blink_duration", s_face.getTuningValue("blink_duration_ms"));
  const int double_blink_chance_pct = s_prefs.getInt(
      "dbl_blink_pct", s_face.getTuningValue("double_blink_chance_pct"));
  const int glow_pulse_amp = s_prefs.getInt(
      "glow_pulse_amp", s_face.getTuningValue("glow_pulse_amp"));
  const int glow_pulse_period_ms = s_prefs.getInt(
      "glow_pulse_ms", s_face.getTuningValue("glow_pulse_period_ms"));
  s_prefs.end();
  switch (face_style) {
    case 0:
    case (int)DESKROBO_STYLE_EVE:
      s_face_style = DESKROBO_STYLE_EVE;
      break;
    case (int)DESKROBO_STYLE_ROUND:
      s_face_style = DESKROBO_STYLE_ROUND;
      break;
    case (int)DESKROBO_STYLE_EVE_SUBTLE:
      s_face_style = DESKROBO_STYLE_EVE_SUBTLE;
      break;
    case (int)DESKROBO_STYLE_EVE_COMIC:
      s_face_style = DESKROBO_STYLE_EVE_COMIC;
      break;
    default:
      s_face_style = DESKROBO_STYLE_EVE;
      break;
  }

  DeskRobo_SetTuning("drift_amp_px", drift_amp_px);
  DeskRobo_SetTuning("saccade_amp_px", saccade_amp_px);
  DeskRobo_SetTuning("saccade_min_ms", saccade_min_ms);
  DeskRobo_SetTuning("saccade_max_ms", saccade_max_ms);
  DeskRobo_SetTuning("blink_interval_ms", blink_interval_ms);
  DeskRobo_SetTuning("blink_duration_ms", blink_duration_ms);
  DeskRobo_SetTuning("double_blink_chance_pct", double_blink_chance_pct);
  DeskRobo_SetTuning("glow_pulse_amp", glow_pulse_amp);
  DeskRobo_SetTuning("glow_pulse_period_ms", glow_pulse_period_ms);
  apply_emotion_style();
  return true;
}

bool DeskRobo_SetStyleByName(const char *name) {
  DeskRoboFaceStyle new_style;
  if (!parse_style_name(name, new_style)) return false;
  s_face_style = new_style;
  apply_emotion_style();
  return true;
}

const char *DeskRobo_GetStyleName() { return style_name(s_face_style); }

void DeskRobo_SetStatusLabelVisible(bool visible) {
  s_status_label_visible = visible;
  if (!s_status) return;
  if (s_status_label_visible) {
    lv_obj_clear_flag(s_status, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(s_status, LV_OBJ_FLAG_HIDDEN);
  }
}

bool DeskRobo_GetStatusLabelVisible() { return s_status_label_visible; }

void DeskRobo_SetBleConnected(bool connected) {
  if (s_ble_state_seen && (s_ble_connected == connected)) return;

  s_ble_state_seen = true;
  s_ble_connected = connected;

  if (connected) {
    const uint32_t now = millis();
    s_ble_disconnected_since_ms = 0;
    s_ble_sleep_face_active = false;
    mark_interaction(now);

    s_eye_pair_active = false;
    s_current_emotion = DESKROBO_EMOTION_IDLE;
    s_current_priority = 0;
    s_emotion_expiry_ms = 0;
    apply_emotion_style();

    Serial.println("[MVP] BLE connected -> wake display");
    return;
  }

  s_ble_disconnected_since_ms = millis();
  s_ble_sleep_face_active = false;
  Serial.println("[MVP] BLE disconnected -> screensaver armed");
}

void DeskRobo_PushEvent(DeskRoboEventType event_type) {
  const uint32_t now = millis();
  const bool is_motion_wake = (event_type == DESKROBO_EVENT_MOTION_SHAKE) ||
                              (event_type == DESKROBO_EVENT_MOTION_TILT) ||
                              (event_type == DESKROBO_EVENT_MOTION_TAP);
  const bool is_user_interaction = is_motion_wake ||
                                   (event_type == DESKROBO_EVENT_PC_CALL) ||
                                   (event_type == DESKROBO_EVENT_PC_MAIL) ||
                                   (event_type == DESKROBO_EVENT_PC_TEAMS);

  if (is_user_interaction) {
    mark_interaction(now);
  }

  if (event_type == DESKROBO_EVENT_MOTION_TAP) {
    s_time_expiry_ms = now + 30000;
    if (s_time_label) lv_obj_clear_flag(s_time_label, LV_OBJ_FLAG_HIDDEN);
  }

  // While screensaver is active, only explicit interaction may interrupt it.
  if (s_ble_sleep_face_active && !is_user_interaction &&
      (event_type != DESKROBO_EVENT_MOTION_TAP)) {
    return;
  }

  const DeskRoboEventRule rule = event_rule(event_type);
  if ((rule.priority < s_current_priority) && (now < s_emotion_expiry_ms)) {
    return;
  }

  s_current_emotion = rule.emotion;
  s_current_priority = rule.priority;
  s_emotion_expiry_ms = now + rule.ttl_ms;
  apply_emotion_style();
}

static void read_audio_sensor() {
#if DESKROBO_AUDIO_ADC_PIN >= 0
  const int32_t sample = analogReadMilliVolts(DESKROBO_AUDIO_ADC_PIN);
  if (s_audio_baseline == 0) {
    s_audio_baseline = sample;
  }

  // Slow baseline + fast envelope for loudness.
  s_audio_baseline = (s_audio_baseline * 31 + sample) / 32;
  const int32_t diff = abs(sample - s_audio_baseline);
  s_audio_level = (s_audio_level * 7 + diff) / 8;

  if (s_audio_level > 170) {
    DeskRobo_PushEvent(DESKROBO_EVENT_AUDIO_VERY_LOUD);
  } else if (s_audio_level > 90) {
    DeskRobo_PushEvent(DESKROBO_EVENT_AUDIO_LOUD);
  } else if (s_audio_level < 20) {
    DeskRobo_PushEvent(DESKROBO_EVENT_AUDIO_QUIET);
  }
#endif
}

static void read_motion_sensor() {
#if DESKROBO_ENABLE_GYRO
  if (!QMI8658_IsReady()) {
    if (s_motion_debug) {
      lv_label_set_text(s_motion_debug, "gyro not detected");
    }
    return;
  }
  QMI8658_Loop();
  if (!isfinite(Accel.x) || !isfinite(Accel.y) || !isfinite(Accel.z)) {
    if (s_motion_debug) {
      lv_label_set_text(s_motion_debug, "gyro invalid sample");
    }
    return;
  }
  const float abs_x = fabsf(Accel.x);
  const float abs_y = fabsf(Accel.y);
  const float abs_z = fabsf(Accel.z);
  static bool has_prev = false;
  static float prev_x = 0.0f;
  static float prev_y = 0.0f;
  static float prev_z = 0.0f;
  const float jerk = has_prev ? (fabsf(Accel.x - prev_x) + fabsf(Accel.y - prev_y) + fabsf(Accel.z - prev_z)) : 0.0f;
  const uint32_t now = millis();

  // Tilt: board moved away from flat orientation.
  if (((abs_x > 0.45f) || (abs_y > 0.45f) || (abs_z < 0.78f)) && ((now - s_last_tilt_event_ms) > 700)) {
    s_last_tilt_event_ms = now;
    s_last_motion_event = "tilt";
#if DESKROBO_GYRO_EVENTS
    DeskRobo_PushEvent(DESKROBO_EVENT_MOTION_TILT);
#endif
  }

  if ((jerk > 1.40f) && ((now - s_last_shake_event_ms) > 2000)) {
    s_last_shake_event_ms = now;
    s_last_motion_event = "shake";
#if DESKROBO_GYRO_EVENTS
    DeskRobo_PushEvent(DESKROBO_EVENT_MOTION_SHAKE);
#endif
  }

  // Tap: sudden acceleration change but not quite a shake.
  static uint32_t s_last_tap_event_ms = 0;
  if ((jerk > 0.50f && jerk < 1.6f) && ((now - s_last_tap_event_ms) > 500) && ((now - s_last_shake_event_ms) > 1000)) {
    s_last_tap_event_ms = now;
    s_last_motion_event = "tap";
#if DESKROBO_GYRO_EVENTS
    DeskRobo_PushEvent(DESKROBO_EVENT_MOTION_TAP);
#endif
  }

  has_prev = true;
  prev_x = Accel.x;
  prev_y = Accel.y;
  prev_z = Accel.z;

  if (s_motion_debug) {
    lv_label_set_text_fmt(
        s_motion_debug,
        "ax %.2f ay %.2f az %.2f\njerk %.2f ev %s",
        Accel.x, Accel.y, Accel.z, jerk, s_last_motion_event);
  }
#else
  if (s_motion_debug) {
    lv_label_set_text(s_motion_debug, "gyro disabled");
  }
#endif
}

void DeskRobo_Init() {
#if DESKROBO_AUDIO_ADC_PIN >= 0
  analogReadResolution(12);
#endif

  lv_obj_clean(lv_scr_act());
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x06080D), 0);
  lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

  s_face.init(lv_scr_act());
  s_status = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(s_status, lv_color_hex(0x8A8F99), 0);
  lv_obj_align(s_status, LV_ALIGN_BOTTOM_MID, 0, -14);

  s_time_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(s_time_label, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(s_time_label, &lv_font_montserrat_16, 0);
  lv_obj_align(s_time_label, LV_ALIGN_CENTER, 0, -60);
  lv_obj_add_flag(s_time_label, LV_OBJ_FLAG_HIDDEN);

#if DESKROBO_SHOW_DEBUG
  s_motion_debug = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(s_motion_debug, lv_color_hex(0x5F6B89), 0);
  lv_obj_set_style_text_font(s_motion_debug, &lv_font_montserrat_12, 0);
  lv_obj_align(s_motion_debug, LV_ALIGN_TOP_LEFT, 10, 8);
#endif

  s_current_emotion = DESKROBO_EMOTION_IDLE;
  s_current_priority = 0;
  s_emotion_expiry_ms = 0;

  s_ble_connected = false;
  s_ble_state_seen = false;
  s_ble_sleep_face_active = false;
  s_display_sleep_off = false;
  s_display_dimmed = false;
  s_ble_disconnected_since_ms = 0;
  s_backlight_before_sleep =
      (LCD_Backlight > 0) ? LCD_Backlight : DESKROBO_DEFAULT_WAKE_BACKLIGHT;
  s_last_interaction_ms = millis();
  s_last_idle_round_ms = s_last_interaction_ms;
  s_idle_round_index = 0;

  DeskRobo_LoadTuning();
  apply_emotion_style();
}

DeskRoboEmotion DeskRobo_GetEmotion() { return s_current_emotion; }

void DeskRobo_Loop() {
  const uint32_t now = millis();

  if (!s_ble_connected) {
    apply_ble_disconnect_policy(now);
  }

  if ((s_current_priority > 0) && (now > s_emotion_expiry_ms)) {
    s_eye_pair_active = false;
    s_current_emotion = DESKROBO_EMOTION_IDLE;
    s_current_priority = 0;
    s_emotion_expiry_ms = 0;
    apply_emotion_style();
  }

  // IDLE stays default; after long inactivity, briefly rotate through a few emotions.
  if (!s_eye_pair_active && !s_ble_sleep_face_active &&
      (s_current_emotion == DESKROBO_EMOTION_IDLE) && (s_current_priority == 0) &&
      ((now - s_last_interaction_ms) >= DESKROBO_IDLE_ROUND_ROBIN_AFTER_MS) &&
      ((now - s_last_idle_round_ms) >= DESKROBO_IDLE_ROUND_ROBIN_INTERVAL_MS)) {
    static const DeskRoboEmotion kIdleRound[] = {
        DESKROBO_EMOTION_HAPPY,
        DESKROBO_EMOTION_CONFUSED,
        DESKROBO_EMOTION_WOW,
        DESKROBO_EMOTION_EXCITED,
        DESKROBO_EMOTION_SAD,
    };
    const size_t idx = (size_t)(s_idle_round_index % (sizeof(kIdleRound) / sizeof(kIdleRound[0])));
    s_idle_round_index = (uint8_t)(s_idle_round_index + 1U);
    s_last_idle_round_ms = now;
    s_current_emotion = kIdleRound[idx];
    s_current_priority = 5;
    s_emotion_expiry_ms = now + DESKROBO_IDLE_ROUND_ROBIN_SHOW_MS;
    apply_emotion_style();
  }

  if ((now - s_last_sensor_ms) >= 80) {
    s_last_sensor_ms = now;
    read_audio_sensor();
  }

  if ((now - s_last_motion_ms) >= 90) {
    s_last_motion_ms = now;
    read_motion_sensor();
  }

  static uint32_t last_time_update = 0;
  if ((s_time_expiry_ms > 0) && (now - last_time_update >= 1000)) {
    last_time_update = now;
    if (now > s_time_expiry_ms) {
      if (s_time_label) lv_obj_add_flag(s_time_label, LV_OBJ_FLAG_HIDDEN);
      s_time_expiry_ms = 0;
    } else {
      struct tm timeinfo;
      if (getLocalTime(&timeinfo, 10)) {
        if (s_time_label) {
          lv_label_set_text_fmt(s_time_label, "%02d:%02d:%02d", timeinfo.tm_hour,
                                timeinfo.tm_min, timeinfo.tm_sec);
          lv_obj_move_foreground(s_time_label);
        }
      } else {
        if (s_time_label) {
          lv_label_set_text(s_time_label, "No Sync");
          lv_obj_move_foreground(s_time_label);
        }
      }
    }
  }

  s_face.update();
}

