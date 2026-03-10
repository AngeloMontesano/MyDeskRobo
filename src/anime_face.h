#pragma once

#include <Arduino.h>
#include <lvgl.h>

#define DISPLAY_WIDTH  360
#define DISPLAY_HEIGHT 360
#define CX (DISPLAY_WIDTH / 2)
#define CY (DISPLAY_HEIGHT / 2)

typedef enum {
  EMOTION_IDLE = 0,
  EMOTION_HAPPY,
  EMOTION_SAD,
  EMOTION_ANGRY,
  EMOTION_ANGST,
  EMOTION_WOW,
  EMOTION_SLEEPY,
  EMOTION_LOVE,
  EMOTION_CONFUSED,
  EMOTION_EXCITED,
  EMOTION_ANRUF,
  EMOTION_LAUT,
  EMOTION_MAIL,
  EMOTION_DENKEN,
  EMOTION_WINK,
  EMOTION_GLITCH,
  EMOTION_COUNT
} Emotion;

typedef enum {
  FACE_STYLE_CLASSIC = 0,
  FACE_STYLE_EVE_CINEMATIC = 1,
  FACE_STYLE_WALLE = 2,
} FaceStyle;

class AnimeFace {
 public:
  struct Tuning {
    int drift_amp_px;
    int saccade_amp_px;
    int saccade_min_ms;
    int saccade_max_ms;
    int blink_interval_ms;
    int blink_duration_ms;
    int double_blink_chance_pct;
    int glow_pulse_amp;
    int glow_pulse_period_ms;
  };

  AnimeFace();
  void init(lv_obj_t *parent);
  void setEmotion(Emotion e);
  void setEyePair(Emotion left, Emotion right);
  void clearEyePair();
  void setStyle(FaceStyle style);
  FaceStyle getStyle() const { return style_; }
  bool setTuningValue(const char *key, int value);
  int getTuningValue(const char *key) const;
  String getTuningJson() const;
  void update();
  Emotion getEmotion() const { return current_; }

 private:
  Emotion current_;
  lv_obj_t *canvas_;
  lv_obj_t *parent_;
  uint32_t last_anim_ms_;
  uint32_t blink_until_ms_;
  uint32_t next_blink_ms_;
  uint32_t next_saccade_ms_;
  int8_t drift_dx_;
  int8_t drift_dy_;
  int8_t saccade_dx_;
  int8_t saccade_dy_;
  int8_t pulse_shift_;
  bool use_pair_override_;
  Emotion left_override_;
  Emotion right_override_;
  FaceStyle style_;
  Tuning tuning_;

  void drawFace();
  void drawEye(int16_t cx, int16_t cy, Emotion e, bool isRight, bool blink);
};
