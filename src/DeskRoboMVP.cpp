#include "DeskRoboMVP.h"

#include <lvgl.h>
#include <math.h>

#include "LVGL_Arduino/Gyro_QMI8658.h"
#include "anime_face.h"

#ifndef DESKROBO_ENABLE_GYRO
#define DESKROBO_ENABLE_GYRO 0
#endif

#ifndef DESKROBO_GYRO_EVENTS
#define DESKROBO_GYRO_EVENTS 0
#endif

#ifndef DESKROBO_SHOW_DEBUG
#define DESKROBO_SHOW_DEBUG 0
#endif

// Set to the analog mic pin if available on your board. -1 disables local mic.
#ifndef DESKROBO_AUDIO_ADC_PIN
#define DESKROBO_AUDIO_ADC_PIN (-1)
#endif

typedef struct {
  DeskRoboEmotion emotion;
  uint8_t priority;
  uint32_t ttl_ms;
} DeskRoboEventRule;

static AnimeFace s_face;
static lv_obj_t *s_status = nullptr;
static lv_obj_t *s_motion_debug = nullptr;

static DeskRoboEmotion s_current_emotion = DESKROBO_EMOTION_IDLE;
static uint8_t s_current_priority = 0;
static uint32_t s_emotion_expiry_ms = 0;

static uint32_t s_last_sensor_ms = 0;
static uint32_t s_last_motion_ms = 0;
static uint32_t s_last_tilt_event_ms = 0;
static uint32_t s_last_shake_event_ms = 0;
static const char *s_last_motion_event = "none";

static int32_t s_audio_baseline = 0;
static int32_t s_audio_level = 0;

static Emotion to_anime(DeskRoboEmotion emotion) {
  switch (emotion) {
    case DESKROBO_EMOTION_HAPPY: return EMOTION_HAPPY;
    case DESKROBO_EMOTION_SAD: return EMOTION_SAD;
    case DESKROBO_EMOTION_ANGRY: return EMOTION_ANGRY;
    case DESKROBO_EMOTION_ANGST: return EMOTION_ANGST;
    case DESKROBO_EMOTION_WOW: return EMOTION_WOW;
    case DESKROBO_EMOTION_SLEEPY: return EMOTION_SLEEPY;
    case DESKROBO_EMOTION_LOVE: return EMOTION_LOVE;
    case DESKROBO_EMOTION_CONFUSED: return EMOTION_CONFUSED;
    case DESKROBO_EMOTION_EXCITED: return EMOTION_EXCITED;
    case DESKROBO_EMOTION_ANRUF: return EMOTION_ANRUF;
    case DESKROBO_EMOTION_LAUT: return EMOTION_LAUT;
    case DESKROBO_EMOTION_MAIL: return EMOTION_MAIL;
    case DESKROBO_EMOTION_DENKEN: return EMOTION_DENKEN;
    case DESKROBO_EMOTION_WINK: return EMOTION_WINK;
    case DESKROBO_EMOTION_GLITCH: return EMOTION_GLITCH;
    case DESKROBO_EMOTION_IDLE:
    default: return EMOTION_IDLE;
  }
}

static const char *emotion_name(DeskRoboEmotion emotion) {
  switch (emotion) {
    case DESKROBO_EMOTION_HAPPY: return "HAPPY";
    case DESKROBO_EMOTION_SAD: return "SAD";
    case DESKROBO_EMOTION_ANGRY: return "ANGRY";
    case DESKROBO_EMOTION_ANGST: return "ANGST";
    case DESKROBO_EMOTION_WOW: return "WOW";
    case DESKROBO_EMOTION_SLEEPY: return "SLEEPY";
    case DESKROBO_EMOTION_LOVE: return "LOVE";
    case DESKROBO_EMOTION_CONFUSED: return "CONFUSED";
    case DESKROBO_EMOTION_EXCITED: return "EXCITED";
    case DESKROBO_EMOTION_ANRUF: return "ANRUF";
    case DESKROBO_EMOTION_LAUT: return "LAUT";
    case DESKROBO_EMOTION_MAIL: return "MAIL";
    case DESKROBO_EMOTION_DENKEN: return "DENKEN";
    case DESKROBO_EMOTION_WINK: return "WINK";
    case DESKROBO_EMOTION_GLITCH: return "GLITCH";
    case DESKROBO_EMOTION_IDLE:
    default: return "IDLE";
  }
}

static DeskRoboEventRule event_rule(DeskRoboEventType event_type) {
  switch (event_type) {
    case DESKROBO_EVENT_PC_CALL: return {DESKROBO_EMOTION_ANRUF, 90, 8000};
    case DESKROBO_EVENT_PC_TEAMS: return {DESKROBO_EMOTION_DENKEN, 80, 5000};
    case DESKROBO_EVENT_PC_MAIL: return {DESKROBO_EMOTION_MAIL, 70, 3500};
    case DESKROBO_EVENT_MOTION_SHAKE: return {DESKROBO_EMOTION_EXCITED, 60, 1200};
    case DESKROBO_EVENT_AUDIO_VERY_LOUD: return {DESKROBO_EMOTION_WOW, 50, 1800};
    case DESKROBO_EVENT_AUDIO_LOUD: return {DESKROBO_EMOTION_LAUT, 40, 1400};
    case DESKROBO_EVENT_MOTION_TILT: return {DESKROBO_EMOTION_CONFUSED, 30, 1200};
    case DESKROBO_EVENT_AUDIO_QUIET: return {DESKROBO_EMOTION_SLEEPY, 10, 1200};
    case DESKROBO_EVENT_NONE:
    default: return {DESKROBO_EMOTION_IDLE, 0, 0};
  }
}

static void apply_emotion_style() {
  s_face.setEmotion(to_anime(s_current_emotion));
  lv_label_set_text_fmt(s_status, "DeskRobo: %s", emotion_name(s_current_emotion));
}

void DeskRobo_SetEmotion(DeskRoboEmotion emotion, uint32_t hold_ms) {
  s_current_emotion = emotion;
  s_current_priority = 100;
  s_emotion_expiry_ms = millis() + hold_ms;
  apply_emotion_style();
}

void DeskRobo_PushEvent(DeskRoboEventType event_type) {
  const DeskRoboEventRule rule = event_rule(event_type);
  if ((rule.priority < s_current_priority) && (millis() < s_emotion_expiry_ms)) {
    return;
  }

  s_current_emotion = rule.emotion;
  s_current_priority = rule.priority;
  s_emotion_expiry_ms = millis() + rule.ttl_ms;
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
  QMI8658_Loop();
  if (!isfinite(Accel.x) || !isfinite(Accel.y) || !isfinite(Accel.z)) {
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

  // Shake: sudden acceleration change.
  if ((jerk > 1.80f) && ((now - s_last_shake_event_ms) > 2000)) {
    s_last_shake_event_ms = now;
    s_last_motion_event = "shake";
#if DESKROBO_GYRO_EVENTS
    DeskRobo_PushEvent(DESKROBO_EVENT_MOTION_SHAKE);
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

#if DESKROBO_SHOW_DEBUG
  s_motion_debug = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(s_motion_debug, lv_color_hex(0x5F6B89), 0);
  lv_obj_set_style_text_font(s_motion_debug, &lv_font_montserrat_12, 0);
  lv_obj_align(s_motion_debug, LV_ALIGN_TOP_LEFT, 10, 8);
#endif

  s_current_emotion = DESKROBO_EMOTION_IDLE;
  s_current_priority = 0;
  s_emotion_expiry_ms = 0;
  apply_emotion_style();
}

DeskRoboEmotion DeskRobo_GetEmotion() {
  return s_current_emotion;
}

void DeskRobo_Loop() {
  const uint32_t now = millis();

  if ((s_current_priority > 0) && (now > s_emotion_expiry_ms)) {
    s_current_emotion = DESKROBO_EMOTION_IDLE;
    s_current_priority = 0;
    s_emotion_expiry_ms = 0;
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

  s_face.update();
}
