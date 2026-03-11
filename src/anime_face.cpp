#include "anime_face.h"
#include <cstring>

AnimeFace::AnimeFace()
    : current_(EMOTION_IDLE),
      canvas_(nullptr),
      parent_(nullptr),
      last_anim_ms_(0),
      blink_until_ms_(0),
      next_blink_ms_(0),
      next_saccade_ms_(0),
      drift_dx_(0),
      drift_dy_(0),
      saccade_dx_(0),
      saccade_dy_(0),
      pulse_shift_(0),
      use_pair_override_(false),
      left_override_(EMOTION_IDLE),
      right_override_(EMOTION_IDLE),
      style_(FACE_STYLE_EVE) {
  tuning_.drift_amp_px = 2;
  tuning_.saccade_amp_px = 4;
  tuning_.saccade_min_ms = 900;
  tuning_.saccade_max_ms = 2200;
  tuning_.blink_interval_ms = 3900;
  tuning_.blink_duration_ms = 120;
  tuning_.double_blink_chance_pct = 24;
  tuning_.glow_pulse_amp = 20;
  tuning_.glow_pulse_period_ms = 1800;
}

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
  use_pair_override_ = false;
  if (canvas_) {
    lv_obj_clean(canvas_);
    drawFace();
  }
}

void AnimeFace::setEyePair(Emotion left, Emotion right) {
  left_override_ = left;
  right_override_ = right;
  use_pair_override_ = true;
  if (canvas_) {
    lv_obj_clean(canvas_);
    drawFace();
  }
}

void AnimeFace::clearEyePair() {
  use_pair_override_ = false;
  if (canvas_) {
    lv_obj_clean(canvas_);
    drawFace();
  }
}

void AnimeFace::setStyle(FaceStyle style) {
  style_ = style;
  if (canvas_) {
    lv_obj_clean(canvas_);
    drawFace();
  }
}

bool AnimeFace::setTuningValue(const char *key, int value) {
  if (!key) return false;
  if (strcmp(key, "drift_amp_px") == 0) tuning_.drift_amp_px = constrain(value, 0, 12);
  else if (strcmp(key, "saccade_amp_px") == 0) tuning_.saccade_amp_px = constrain(value, 0, 18);
  else if (strcmp(key, "saccade_min_ms") == 0) tuning_.saccade_min_ms = constrain(value, 200, 10000);
  else if (strcmp(key, "saccade_max_ms") == 0) tuning_.saccade_max_ms = constrain(value, 250, 12000);
  else if (strcmp(key, "blink_interval_ms") == 0) tuning_.blink_interval_ms = constrain(value, 400, 12000);
  else if (strcmp(key, "blink_duration_ms") == 0) tuning_.blink_duration_ms = constrain(value, 40, 800);
  else if (strcmp(key, "double_blink_chance_pct") == 0) tuning_.double_blink_chance_pct = constrain(value, 0, 100);
  else if (strcmp(key, "glow_pulse_amp") == 0) tuning_.glow_pulse_amp = constrain(value, 0, 80);
  else if (strcmp(key, "glow_pulse_period_ms") == 0) tuning_.glow_pulse_period_ms = constrain(value, 200, 8000);
  else return false;

  if (tuning_.saccade_max_ms < tuning_.saccade_min_ms) {
    tuning_.saccade_max_ms = tuning_.saccade_min_ms;
  }
  return true;
}

int AnimeFace::getTuningValue(const char *key) const {
  if (!key) return 0;
  if (strcmp(key, "drift_amp_px") == 0) return tuning_.drift_amp_px;
  if (strcmp(key, "saccade_amp_px") == 0) return tuning_.saccade_amp_px;
  if (strcmp(key, "saccade_min_ms") == 0) return tuning_.saccade_min_ms;
  if (strcmp(key, "saccade_max_ms") == 0) return tuning_.saccade_max_ms;
  if (strcmp(key, "blink_interval_ms") == 0) return tuning_.blink_interval_ms;
  if (strcmp(key, "blink_duration_ms") == 0) return tuning_.blink_duration_ms;
  if (strcmp(key, "double_blink_chance_pct") == 0) return tuning_.double_blink_chance_pct;
  if (strcmp(key, "glow_pulse_amp") == 0) return tuning_.glow_pulse_amp;
  if (strcmp(key, "glow_pulse_period_ms") == 0) return tuning_.glow_pulse_period_ms;
  return 0;
}

String AnimeFace::getTuningJson() const {
  String out = "{";
  out += "\"drift_amp_px\":" + String(tuning_.drift_amp_px);
  out += ",\"saccade_amp_px\":" + String(tuning_.saccade_amp_px);
  out += ",\"saccade_min_ms\":" + String(tuning_.saccade_min_ms);
  out += ",\"saccade_max_ms\":" + String(tuning_.saccade_max_ms);
  out += ",\"blink_interval_ms\":" + String(tuning_.blink_interval_ms);
  out += ",\"blink_duration_ms\":" + String(tuning_.blink_duration_ms);
  out += ",\"double_blink_chance_pct\":" + String(tuning_.double_blink_chance_pct);
  out += ",\"glow_pulse_amp\":" + String(tuning_.glow_pulse_amp);
  out += ",\"glow_pulse_period_ms\":" + String(tuning_.glow_pulse_period_ms);
  out += "}";
  return out;
}

void AnimeFace::update() {
  const uint32_t now = millis();
  if ((now - last_anim_ms_) < 33) {
    return;
  }
  last_anim_ms_ = now;

  if (next_blink_ms_ == 0) {
    next_blink_ms_ = now + (uint32_t) tuning_.blink_interval_ms;
  }
  if (next_saccade_ms_ == 0) {
    next_saccade_ms_ = now + (uint32_t) random(tuning_.saccade_min_ms, tuning_.saccade_max_ms + 1);
  }

  if (now >= next_blink_ms_) {
    blink_until_ms_ = now + (uint32_t) tuning_.blink_duration_ms;
    next_blink_ms_ = now + (uint32_t) tuning_.blink_interval_ms;
    if (random(0, 100) < tuning_.double_blink_chance_pct) {
      next_blink_ms_ += 140;
    }
  }
  const bool blink = (now < blink_until_ms_);

  if (now >= next_saccade_ms_) {
    saccade_dx_ = random(-tuning_.saccade_amp_px, tuning_.saccade_amp_px + 1);
    saccade_dy_ = random(-tuning_.saccade_amp_px / 2, tuning_.saccade_amp_px / 2 + 1);
    next_saccade_ms_ = now + (uint32_t) random(tuning_.saccade_min_ms, tuning_.saccade_max_ms + 1);
  }

  if ((now % 700) < 33) {
    drift_dx_ = random(-tuning_.drift_amp_px, tuning_.drift_amp_px + 1);
    drift_dy_ = random(-tuning_.drift_amp_px, tuning_.drift_amp_px + 1);
  }

  const float phase = (float) (now % (uint32_t) tuning_.glow_pulse_period_ms) / (float) tuning_.glow_pulse_period_ms;
  pulse_shift_ = (int8_t) (sinf(phase * 6.2831853f) * (float) tuning_.glow_pulse_amp);

  const Emotion left = use_pair_override_ ? left_override_ : current_;
  const Emotion right = use_pair_override_ ? right_override_ : current_;

  const int16_t eye_gap = (style_ == FACE_STYLE_PLAYFUL) ? 60 : ((style_ == FACE_STYLE_ROBO) ? 56 : 68);
  lv_obj_clean(canvas_);
  drawEye(CX - eye_gap + drift_dx_ + saccade_dx_, CY + drift_dy_ + saccade_dy_, left, false, blink);
  drawEye(CX + eye_gap + drift_dx_ + saccade_dx_, CY + drift_dy_ + saccade_dy_, right, true, blink);
}

void AnimeFace::drawFace() {
  const Emotion left = use_pair_override_ ? left_override_ : current_;
  const Emotion right = use_pair_override_ ? right_override_ : current_;
  const int16_t eye_gap = (style_ == FACE_STYLE_PLAYFUL) ? 60 : ((style_ == FACE_STYLE_ROBO) ? 56 : 68);
  drawEye(CX - eye_gap, CY, left, false, false);
  drawEye(CX + eye_gap, CY, right, true, false);
}

void AnimeFace::drawEye(int16_t cx, int16_t cy, Emotion e, bool isRight, bool blink) {
  const bool playful = (style_ == FACE_STYLE_PLAYFUL);
  const bool robo = (style_ == FACE_STYLE_ROBO);
  int16_t w = robo ? 52 : (playful ? 62 : 54);
  int16_t h = robo ? 78 : (playful ? 92 : 110);
  lv_color_t color = robo ? lv_color_make(0x2D, 0xDC, 0xF0)
                          : (playful ? lv_color_make(0xF5, 0xD5, 0x74) : lv_color_make(0x0D, 0xC8, 0xF6));
  bool carve_top = false;
  int16_t carve_h = 0;
  int16_t carve_dx = 0;
  bool inner_clip = false;
  int16_t clip_w = 0;
  bool carve_bottom = false;
  int16_t carve_bottom_h = 0;
  bool add_side_shadow = !robo;
  const lv_color_t bg = lv_color_hex(0x06080D);
  const uint8_t idle_phase = (uint8_t) ((millis() / 2400U) % 4U);
  Emotion mapped = e;
  int16_t extra_w = 0;
  int16_t extra_h = 0;
  int16_t extra_x = 0;
  int16_t extra_y = 0;
  bool force_no_shadow = false;

  // Additional preset pack inspired by esp32-eyes (implemented independently).
  switch (e) {
    case EMOTION_ESP_NORMAL:
      mapped = EMOTION_IDLE;
      break;
    case EMOTION_ESP_TIRED:
      mapped = EMOTION_SLEEPY;
      extra_h = -2;
      extra_y = 6;
      break;
    case EMOTION_ESP_GLEE:
      mapped = EMOTION_HAPPY;
      extra_w = 4;
      break;
    case EMOTION_ESP_WORRIED:
      mapped = EMOTION_SAD;
      extra_h = 8;
      break;
    case EMOTION_ESP_FOCUSED:
      mapped = EMOTION_DENKEN;
      extra_h = 10;
      break;
    case EMOTION_ESP_ANNOYED:
      mapped = EMOTION_ANGRY;
      extra_h = -6;
      break;
    case EMOTION_ESP_SURPRISED:
      mapped = EMOTION_WOW;
      extra_w = 4;
      break;
    case EMOTION_ESP_SKEPTIC:
      mapped = EMOTION_CONFUSED;
      extra_y = isRight ? -4 : 4;
      break;
    case EMOTION_ESP_FRUSTRATED:
      mapped = EMOTION_ANGRY;
      extra_w = 4;
      extra_h = 6;
      break;
    case EMOTION_ESP_UNIMPRESSED:
      mapped = EMOTION_SLEEPY;
      extra_h = -1;
      break;
    case EMOTION_ESP_SUSPICIOUS:
      mapped = EMOTION_ANGRY;
      extra_w = -2;
      extra_y = 2;
      break;
    case EMOTION_ESP_SQUINT:
      mapped = EMOTION_SLEEPY;
      extra_h = -3;
      break;
    case EMOTION_ESP_FURIOUS:
      mapped = EMOTION_ANGRY;
      extra_w = 6;
      extra_h = 2;
      break;
    case EMOTION_ESP_SCARED:
      mapped = EMOTION_ANGST;
      extra_h = 12;
      break;
    case EMOTION_ESP_AWE:
      mapped = EMOTION_WOW;
      extra_h = 8;
      force_no_shadow = true;
      break;
    default:
      break;
  }

  if (blink && (mapped != EMOTION_WOW) && (mapped != EMOTION_GLITCH)) {
    h = playful ? 10 : 8;
  }

  switch (mapped) {
    case EMOTION_HAPPY:
      h = blink ? h : (playful ? 62 : 74);
      cy += 6;
      carve_top = !blink;
      carve_h = playful ? 14 : 18;
      carve_dx = isRight ? -10 : 10;
      break;
    case EMOTION_SAD:
      h = blink ? h : 62;
      w = playful ? 58 : 54;
      cy += 20;
      inner_clip = !blink;
      clip_w = 12;
      break;
    case EMOTION_ANGRY:
      h = blink ? h : 62;
      w = playful ? 58 : 54;
      cy += 8;
      inner_clip = !blink;
      clip_w = 20;
      break;
    case EMOTION_ANGST:
      w = playful ? 48 : 40;
      h = blink ? h : 60;
      break;
    case EMOTION_WOW:
      w = playful ? 58 : 52;
      h = playful ? 102 : 98;
      add_side_shadow = true;
      break;
    case EMOTION_SLEEPY:
      h = blink ? h : 10;
      cy += 15;
      add_side_shadow = false;
      break;
    case EMOTION_LOVE:
      h = blink ? h : (playful ? 70 : 74);
      w = playful ? 56 : 50;
      carve_top = !blink;
      carve_h = playful ? 20 : 24;
      carve_dx = isRight ? -4 : 4;
      carve_bottom = !blink;
      carve_bottom_h = playful ? 14 : 16;
      break;
    case EMOTION_CONFUSED:
      if (isRight) {
        cy -= 15;
      } else {
        cy += 15;
      }
      w = playful ? 60 : 54;
      break;
    case EMOTION_EXCITED:
      w = playful ? 72 : 65;
      h = blink ? h : 65;
      break;
    case EMOTION_ANRUF:
      h = blink ? h : (playful ? 84 : 96);
      w = playful ? 52 : 46;
      cy -= 4;
      color = playful ? lv_color_make(0x6E, 0xE2, 0xFF) : lv_color_make(0x53, 0xDC, 0xFF);
      break;
    case EMOTION_LAUT:
      h = blink ? h : (playful ? 56 : 60);
      w = playful ? 70 : 64;
      color = playful ? lv_color_make(0xFF, 0xB2, 0x6F) : lv_color_make(0x35, 0xD9, 0xFF);
      break;
    case EMOTION_MAIL:
      h = blink ? h : (playful ? 72 : 76);
      w = playful ? 58 : 52;
      cy += 4;
      inner_clip = !blink;
      clip_w = playful ? 10 : 8;
      break;
    case EMOTION_DENKEN:
      w = playful ? 50 : 45;
      h = blink ? h : 45;
      break;
    case EMOTION_WINK:
      if (isRight) {
        h = playful ? 12 : 10;
      } else {
        h = blink ? h : (playful ? 68 : 74);
        carve_top = !blink;
        carve_h = 18;
        carve_dx = 10;
      }
      break;
    case EMOTION_GLITCH:
      // Keep glitch deterministic to avoid full-screen rainbow flashing.
      color = lv_color_make(0x8C, 0x5C, 0xFF);
      cx += isRight ? 4 : -4;
      cy += 2;
      w = playful ? 64 : 54;
      h = playful ? 92 : 104;
      add_side_shadow = false;
      break;
    case EMOTION_LOCKED:
      h = blink ? h : (playful ? 70 : 82);
      w = playful ? 54 : 48;
      cy += 10;
      carve_top = !blink;
      carve_h = playful ? 8 : 10;
      break;
    case EMOTION_WIFI:
      h = blink ? h : (playful ? 80 : 90);
      w = playful ? 60 : 50;
      cy -= 2;
      add_side_shadow = !playful;
      break;
    case EMOTION_IDLE:
      if (idle_phase == 1) {
        h = blink ? h : (playful ? 94 : 98);
        cy += 4;
        carve_top = !blink;
        carve_h = 14;
        carve_dx = isRight ? -10 : 10;
      } else if (idle_phase == 2) {
        h = blink ? h : (playful ? 88 : 90);
        w = playful ? 66 : 62;
        cy += 12;
        add_side_shadow = false;
      } else if (idle_phase == 3) {
        h = blink ? h : (playful ? 96 : 104);
      }
      if (!playful) {
        if (idle_phase == 0) {
          h = blink ? 8 : 112;
          w = 52;
        } else if (idle_phase == 1) {
          h = blink ? 8 : 104;
          w = 58;
          cy += 3;
          carve_h = 12;
        } else if (idle_phase == 2) {
          h = blink ? 8 : 96;
          w = 62;
          cy += 10;
          add_side_shadow = false;
        } else {
          h = blink ? 8 : 108;
          w = 54;
        }
      } else {
        if (idle_phase == 0) {
          h = blink ? 10 : 96;
          w = 62;
        } else if (idle_phase == 1) {
          h = blink ? 10 : 90;
          w = 66;
          cy += 4;
        } else if (idle_phase == 2) {
          h = blink ? 10 : 84;
          w = 68;
          cy += 10;
          add_side_shadow = false;
        } else {
          h = blink ? 10 : 94;
          w = 64;
        }
      }
      break;
    default:
      break;
  }

  if (robo) {
    // RoboEyes test profile: simpler iconic shapes.
    if (mapped == EMOTION_IDLE) {
      w = blink ? 50 : 52;
      h = blink ? 8 : 78;
    } else if (mapped == EMOTION_HAPPY) {
      w = 56;
      h = blink ? 8 : 28;
      cy += 18;
      carve_top = false;
      inner_clip = false;
      add_side_shadow = false;
    } else if (mapped == EMOTION_SAD) {
      w = 50;
      h = blink ? 8 : 56;
      cy += 10;
      inner_clip = !blink;
      clip_w = 14;
    } else if (mapped == EMOTION_ANGRY) {
      w = 52;
      h = blink ? 8 : 48;
      cy += 8;
      inner_clip = !blink;
      clip_w = 20;
    } else if (mapped == EMOTION_WOW) {
      w = 58;
      h = blink ? 8 : 90;
    } else if (mapped == EMOTION_SLEEPY) {
      w = 54;
      h = blink ? 8 : 8;
      cy += 20;
      add_side_shadow = false;
    } else if (mapped == EMOTION_WINK) {
      if (isRight) {
        w = 54;
        h = 8;
        cy += 20;
      } else {
        w = 52;
        h = blink ? 8 : 60;
      }
    }
  }

  w += extra_w;
  h += extra_h;
  cx += extra_x;
  cy += extra_y;
  if (force_no_shadow) add_side_shadow = false;
  if (w < 10) w = 10;
  if (h < 6) h = 6;

  const int glow_pad = robo ? 8 : (playful ? 6 : 8);
  const int mid_pad = robo ? 3 : (playful ? 2 : 3);
  const int glow_base = robo ? 34 : (playful ? 14 : 20);
  const int glow_hi = robo ? 66 : (playful ? 36 : 56);
  const int mid_base = robo ? 88 : (playful ? 62 : 76);
  const int mid_hi = robo ? 150 : (playful ? 92 : 122);
  const lv_coord_t eye_radius = robo ? LV_RADIUS_CIRCLE : (playful ? 12 : LV_RADIUS_CIRCLE);

  lv_obj_t *glow = lv_obj_create(canvas_);
  lv_obj_remove_style_all(glow);
  lv_obj_set_size(glow, w + glow_pad, h + glow_pad);
  lv_obj_set_pos(glow, cx - (w + glow_pad) / 2, cy - (h + glow_pad) / 2);
  lv_obj_set_style_radius(glow, eye_radius, 0);
  lv_obj_set_style_bg_color(glow, color, 0);
  lv_obj_set_style_bg_opa(glow, (lv_opa_t) constrain(glow_base + pulse_shift_ / 4, 6, glow_hi), 0);

  lv_obj_t *mid = lv_obj_create(canvas_);
  lv_obj_remove_style_all(mid);
  lv_obj_set_size(mid, w + mid_pad, h + mid_pad);
  lv_obj_set_pos(mid, cx - (w + mid_pad) / 2, cy - (h + mid_pad) / 2);
  lv_obj_set_style_radius(mid, eye_radius, 0);
  lv_obj_set_style_bg_color(mid, color, 0);
  lv_obj_set_style_bg_opa(mid, (lv_opa_t) constrain(mid_base + pulse_shift_ / 4, 24, mid_hi), 0);

  lv_obj_t *core = lv_obj_create(canvas_);
  lv_obj_remove_style_all(core);
  lv_obj_set_size(core, w, h);
  lv_obj_set_pos(core, cx - w / 2, cy - h / 2);
  lv_obj_set_style_radius(core, eye_radius, 0);
  lv_obj_set_style_bg_color(core, color, 0);
  lv_obj_set_style_bg_opa(core, LV_OPA_COVER, 0);

  if (robo && h > 12 && w > 12) {
    lv_obj_t *inner = lv_obj_create(canvas_);
    lv_obj_remove_style_all(inner);
    const int iw = (w * 62) / 100;
    const int ih = (h * 68) / 100;
    lv_obj_set_size(inner, iw, ih);
    lv_obj_set_pos(inner, cx - iw / 2 + 1, cy - ih / 2 + 1);
    lv_obj_set_style_radius(inner, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(inner, lv_color_make(0xCC, 0xF8, 0xFF), 0);
    lv_obj_set_style_bg_opa(inner, (lv_opa_t) 90, 0);
  }

  if (carve_top && (carve_h > 0)) {
    lv_obj_t *cut = lv_obj_create(canvas_);
    lv_obj_remove_style_all(cut);
    lv_obj_set_size(cut, w + 6, carve_h);
    lv_obj_set_pos(cut, cx - (w + 6) / 2 + carve_dx, cy - h / 2 - 2);
    lv_obj_set_style_bg_color(cut, bg, 0);
    lv_obj_set_style_bg_opa(cut, LV_OPA_COVER, 0);
  }

  if (inner_clip && (clip_w > 0)) {
    lv_obj_t *clip = lv_obj_create(canvas_);
    lv_obj_remove_style_all(clip);
    lv_obj_set_size(clip, clip_w, h / 2 + 4);
    if (isRight) {
      lv_obj_set_pos(clip, cx - w / 2 - 2, cy - h / 2 - 2);
    } else {
      lv_obj_set_pos(clip, cx + w / 2 - clip_w + 2, cy - h / 2 - 2);
    }
    lv_obj_set_style_bg_color(clip, bg, 0);
    lv_obj_set_style_bg_opa(clip, LV_OPA_COVER, 0);
  }

  if (carve_bottom && (carve_bottom_h > 0)) {
    lv_obj_t *cut = lv_obj_create(canvas_);
    lv_obj_remove_style_all(cut);
    lv_obj_set_size(cut, w + 6, carve_bottom_h);
    lv_obj_set_pos(cut, cx - (w + 6) / 2 - carve_dx / 2, cy + h / 2 - carve_bottom_h + 2);
    lv_obj_set_style_bg_color(cut, bg, 0);
    lv_obj_set_style_bg_opa(cut, LV_OPA_COVER, 0);
  }

  if (add_side_shadow) {
    lv_obj_t *shadow = lv_obj_create(canvas_);
    lv_obj_remove_style_all(shadow);
    const int16_t sw = playful ? 8 : 10;
    lv_obj_set_size(shadow, sw, h - 16);
    lv_obj_set_pos(shadow, cx + (isRight ? (w / 2 - sw / 2 + 2) : (-w / 2 - sw / 2 - 2)), cy - (h - 12) / 2);
    lv_obj_set_style_radius(shadow, playful ? 8 : LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(shadow, lv_color_hex(0x1A2538), 0);
    lv_obj_set_style_bg_opa(shadow, (lv_opa_t) (playful ? 38 : 54), 0);
  }
}
