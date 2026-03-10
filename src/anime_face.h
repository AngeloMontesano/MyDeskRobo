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

class AnimeFace {
 public:
  AnimeFace();
  void init(lv_obj_t *parent);
  void setEmotion(Emotion e);
  void update();
  Emotion getEmotion() const { return current_; }

 private:
  Emotion current_;
  lv_obj_t *canvas_;
  lv_obj_t *parent_;
  uint32_t last_anim_ms_;
  uint32_t blink_until_ms_;
  int8_t drift_dx_;
  int8_t drift_dy_;

  void drawFace();
  void drawEye(int16_t cx, int16_t cy, Emotion e, bool isRight, bool blink);
};

