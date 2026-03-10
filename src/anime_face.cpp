#include "anime_face.h"

AnimeFace::AnimeFace()
    : current_(EMOTION_IDLE),
      canvas_(nullptr),
      parent_(nullptr),
      last_anim_ms_(0),
      blink_until_ms_(0),
      drift_dx_(0),
      drift_dy_(0) {}

void AnimeFace::init(lv_obj_t *parent) {
  parent_ = parent;
  canvas_ = lv_obj_create(parent);
  lv_obj_remove_style_all(canvas_);
  lv_obj_set_size(canvas_, DISPLAY_WIDTH, DISPLAY_HEIGHT);
  lv_obj_set_pos(canvas_, 0, 0);
  lv_obj_set_style_bg_color(canvas_, lv_color_hex(0x06080D), 0);
  lv_obj_set_style_bg_opa(canvas_, LV_OPA_COVER, 0);
  drawFace();
}

void AnimeFace::setEmotion(Emotion e) {
  current_ = e;
  if (canvas_) {
    lv_obj_clean(canvas_);
    drawFace();
  }
}

void AnimeFace::update() {
  const uint32_t now = millis();
  if ((now - last_anim_ms_) < 33) {
    return;
  }
  last_anim_ms_ = now;

  if ((now % 4100) < 50) {
    blink_until_ms_ = now + 130;
  }
  const bool blink = (now < blink_until_ms_);

  // Small idle drift so the face feels alive.
  if ((now % 700) < 33) {
    drift_dx_ = random(-2, 3);
    drift_dy_ = random(-2, 3);
  }

  lv_obj_clean(canvas_);
  drawEye(CX - 70 + drift_dx_, CY + drift_dy_, current_, false, blink);
  drawEye(CX + 70 + drift_dx_, CY + drift_dy_, current_, true, blink);
}

void AnimeFace::drawFace() {
  drawEye(CX - 70, CY, current_, false, false);
  drawEye(CX + 70, CY, current_, true, false);
}

void AnimeFace::drawEye(int16_t cx, int16_t cy, Emotion e, bool isRight, bool blink) {
  int16_t w = 55;
  int16_t h = 55;
  lv_color_t color = lv_color_make(0x00, 0xFF, 0xCC);

  if (blink && (e != EMOTION_WOW) && (e != EMOTION_GLITCH)) {
    h = 8;
  }

  switch (e) {
    case EMOTION_HAPPY:
      h = blink ? 8 : 35;
      cy += 10;
      color = lv_color_make(0x00, 0xFF, 0x88);
      break;
    case EMOTION_SAD:
      h = blink ? 8 : 35;
      cy -= 5;
      color = lv_color_make(0x44, 0x88, 0xFF);
      break;
    case EMOTION_ANGRY:
      h = blink ? 8 : 30;
      cy -= 8;
      color = lv_color_make(0xFF, 0x33, 0x00);
      break;
    case EMOTION_ANGST:
      w = 40;
      h = blink ? 8 : 60;
      color = lv_color_make(0xFF, 0x88, 0x00);
      break;
    case EMOTION_WOW:
      w = 70;
      h = 70;
      color = lv_color_make(0xFF, 0xFF, 0x00);
      break;
    case EMOTION_SLEEPY:
      h = blink ? 8 : 20;
      cy += 15;
      color = lv_color_make(0x88, 0x66, 0xFF);
      break;
    case EMOTION_LOVE:
      color = lv_color_make(0xFF, 0x44, 0x88);
      break;
    case EMOTION_CONFUSED:
      if (isRight) {
        cy -= 15;
      } else {
        cy += 15;
      }
      color = lv_color_make(0xFF, 0xAA, 0x00);
      break;
    case EMOTION_EXCITED:
      w = 65;
      h = blink ? 8 : 65;
      color = lv_color_make(0x00, 0xFF, 0xFF);
      break;
    case EMOTION_ANRUF:
      color = lv_color_make(0x00, 0xFF, 0x33);
      break;
    case EMOTION_LAUT:
      color = lv_color_make(0xFF, 0x77, 0x00);
      break;
    case EMOTION_MAIL:
      color = lv_color_make(0x33, 0xCC, 0xFF);
      break;
    case EMOTION_DENKEN:
      w = 45;
      h = blink ? 8 : 45;
      color = lv_color_make(0xAA, 0x44, 0xFF);
      break;
    case EMOTION_WINK:
      if (isRight) {
        h = 8;
      }
      color = lv_color_make(0x00, 0xFF, 0xCC);
      break;
    case EMOTION_GLITCH:
      // Keep glitch deterministic to avoid full-screen rainbow flashing.
      color = lv_color_make(0xFF, 0x20, 0x70);
      cx += isRight ? 4 : -4;
      cy += 2;
      break;
    case EMOTION_IDLE:
    default:
      break;
  }

  lv_obj_t *glow = lv_obj_create(canvas_);
  lv_obj_remove_style_all(glow);
  lv_obj_set_size(glow, w + 30, h + 30);
  lv_obj_set_pos(glow, cx - (w + 30) / 2, cy - (h + 30) / 2);
  lv_obj_set_style_radius(glow, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(glow, color, 0);
  lv_obj_set_style_bg_opa(glow, LV_OPA_30, 0);

  lv_obj_t *mid = lv_obj_create(canvas_);
  lv_obj_remove_style_all(mid);
  lv_obj_set_size(mid, w + 12, h + 12);
  lv_obj_set_pos(mid, cx - (w + 12) / 2, cy - (h + 12) / 2);
  lv_obj_set_style_radius(mid, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(mid, color, 0);
  lv_obj_set_style_bg_opa(mid, LV_OPA_60, 0);

  lv_obj_t *core = lv_obj_create(canvas_);
  lv_obj_remove_style_all(core);
  lv_obj_set_size(core, w, h);
  lv_obj_set_pos(core, cx - w / 2, cy - h / 2);
  lv_obj_set_style_radius(core, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(core, color, 0);
  lv_obj_set_style_bg_opa(core, LV_OPA_COVER, 0);
}
