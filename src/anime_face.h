#pragma once

#include <Arduino.h>
#include <stdint.h>
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
  EMOTION_WOW,
  EMOTION_SLEEPY,
  EMOTION_CONFUSED,
  EMOTION_EXCITED,
  EMOTION_DIZZY,
  EMOTION_MAIL,
  EMOTION_CALL,
  EMOTION_SHAKE,
  EMOTION_ANGRY_1,
  EMOTION_ANGRY_2,
  EMOTION_ANGRY_3,
  EMOTION_ANGRY_4,
  EMOTION_ANGRY_5,
  EMOTION_ANGRY_6,
  EMOTION_ANGRY_7,
  EMOTION_ANGRY_8,
  EMOTION_ANGRY_9,
  EMOTION_ANGRY_10,
  EMOTION_SAD_1,
  EMOTION_SAD_2,
  EMOTION_SAD_3,
  EMOTION_SAD_4,
  EMOTION_SAD_5,
  EMOTION_SAD_6,
  EMOTION_SAD_7,
  EMOTION_SAD_8,
  EMOTION_SAD_9,
  EMOTION_SAD_10,
  EMOTION_COUNT
} Emotion;

typedef enum {
  FACE_STYLE_EVE_CINEMATIC = 0,
  FACE_STYLE_FLUX,
  FACE_STYLE_ANGRY_LAB,
  FACE_STYLE_SAD_LAB
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
    int shake_amp_px;
    int shake_period_ms;
    int eye_color_r;
    int eye_color_g;
    int eye_color_b;
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
  void drawEyeEVE(int16_t cx, int16_t cy, Emotion e, bool isRight, bool blink);
  void drawEyeFlux(int16_t cx, int16_t cy, Emotion e, bool isRight, bool blink);
  void drawEyeMoodLab(int16_t cx, int16_t cy, Emotion e, bool isRight, bool blink);
};
