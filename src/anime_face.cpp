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
      style_(FACE_STYLE_CLASSIC) {
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

  const int16_t eye_gap = (style_ == FACE_STYLE_WALLE) ? 62 : ((style_ == FACE_STYLE_EVE_CINEMATIC) ? 68 : 70);
  lv_obj_clean(canvas_);
  drawEye(CX - eye_gap + drift_dx_ + saccade_dx_, CY + drift_dy_ + saccade_dy_, left, false, blink);
  drawEye(CX + eye_gap + drift_dx_ + saccade_dx_, CY + drift_dy_ + saccade_dy_, right, true, blink);
}

void AnimeFace::drawFace() {
  const Emotion left = use_pair_override_ ? left_override_ : current_;
  const Emotion right = use_pair_override_ ? right_override_ : current_;
  const int16_t eye_gap = (style_ == FACE_STYLE_WALLE) ? 62 : ((style_ == FACE_STYLE_EVE_CINEMATIC) ? 68 : 70);
  drawEye(CX - eye_gap, CY, left, false, false);
  drawEye(CX + eye_gap, CY, right, true, false);
}

void AnimeFace::drawEye(int16_t cx, int16_t cy, Emotion e, bool isRight, bool blink) {
  int16_t w = (style_ == FACE_STYLE_WALLE) ? 52 : ((style_ == FACE_STYLE_EVE_CINEMATIC) ? 54 : 58);
  int16_t h = (style_ == FACE_STYLE_WALLE) ? 96 : ((style_ == FACE_STYLE_EVE_CINEMATIC) ? 110 : 106);
  lv_color_t color = (style_ == FACE_STYLE_EVE_CINEMATIC || style_ == FACE_STYLE_WALLE)
                         ? lv_color_make(0x0D, 0xC8, 0xF6)
                         : lv_color_make(0x10, 0xC9, 0xEE);
  bool carve_top = false;
  int16_t carve_h = 0;
  int16_t carve_dx = 0;
  bool inner_clip = false;
  int16_t clip_w = 0;
  bool add_side_shadow = true;
  const lv_color_t bg = lv_color_hex(0x06080D);
  const uint8_t idle_phase = (uint8_t) ((millis() / 2400U) % 4U);

  if (blink && (e != EMOTION_WOW) && (e != EMOTION_GLITCH)) {
    h = 8;
  }

  switch (e) {
    case EMOTION_HAPPY:
      h = blink ? 8 : 74;
      cy += 6;
      carve_top = !blink;
      carve_h = 18;
      carve_dx = isRight ? -10 : 10;
      break;
    case EMOTION_SAD:
      h = blink ? 8 : 62;
      w = 54;
      cy += 20;
      inner_clip = !blink;
      clip_w = 12;
      break;
    case EMOTION_ANGRY:
      h = blink ? 8 : 62;
      w = 54;
      cy += 8;
      inner_clip = !blink;
      clip_w = 20;
      break;
    case EMOTION_ANGST:
      w = 40;
      h = blink ? 8 : 60;
      break;
    case EMOTION_WOW:
      w = 52;
      h = 98;
      add_side_shadow = true;
      break;
    case EMOTION_SLEEPY:
      h = blink ? 8 : 10;
      cy += 15;
      add_side_shadow = false;
      break;
    case EMOTION_LOVE:
      break;
    case EMOTION_CONFUSED:
      if (isRight) {
        cy -= 15;
      } else {
        cy += 15;
      }
      break;
    case EMOTION_EXCITED:
      w = 65;
      h = blink ? 8 : 65;
      break;
    case EMOTION_ANRUF:
      break;
    case EMOTION_LAUT:
      break;
    case EMOTION_MAIL:
      break;
    case EMOTION_DENKEN:
      w = 45;
      h = blink ? 8 : 45;
      break;
    case EMOTION_WINK:
      if (isRight) {
        h = 10;
      } else {
        h = blink ? 8 : 74;
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
      add_side_shadow = false;
      break;
    case EMOTION_IDLE:
      if (idle_phase == 1) {
        h = blink ? 8 : 98;
        cy += 4;
        carve_top = !blink;
        carve_h = 14;
        carve_dx = isRight ? -10 : 10;
      } else if (idle_phase == 2) {
        h = blink ? 8 : 90;
        w = 62;
        cy += 12;
        add_side_shadow = false;
      } else if (idle_phase == 3) {
        h = blink ? 8 : 104;
      }
      if (style_ == FACE_STYLE_EVE_CINEMATIC) {
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
      } else if (style_ == FACE_STYLE_WALLE) {
        if (idle_phase == 0) {
          h = blink ? 8 : 100;
          w = 50;
        } else if (idle_phase == 1) {
          h = blink ? 8 : 94;
          w = 56;
          cy += 4;
        } else if (idle_phase == 2) {
          h = blink ? 8 : 90;
          w = 58;
          cy += 10;
          add_side_shadow = false;
        } else {
          h = blink ? 8 : 98;
          w = 52;
        }
      }
      break;
    default:
      break;
  }

  const int glow_pad = (style_ == FACE_STYLE_CLASSIC) ? 10 : ((style_ == FACE_STYLE_EVE_CINEMATIC) ? 8 : 7);
  const int mid_pad = (style_ == FACE_STYLE_CLASSIC) ? 4 : ((style_ == FACE_STYLE_EVE_CINEMATIC) ? 3 : 2);
  const int glow_base = (style_ == FACE_STYLE_CLASSIC) ? 28 : ((style_ == FACE_STYLE_EVE_CINEMATIC) ? 20 : 16);
  const int glow_hi = (style_ == FACE_STYLE_CLASSIC) ? 70 : ((style_ == FACE_STYLE_EVE_CINEMATIC) ? 56 : 42);
  const int mid_base = (style_ == FACE_STYLE_CLASSIC) ? 82 : ((style_ == FACE_STYLE_EVE_CINEMATIC) ? 76 : 70);
  const int mid_hi = (style_ == FACE_STYLE_CLASSIC) ? 132 : ((style_ == FACE_STYLE_EVE_CINEMATIC) ? 122 : 104);

  lv_obj_t *glow = lv_obj_create(canvas_);
  lv_obj_remove_style_all(glow);
  lv_obj_set_size(glow, w + glow_pad, h + glow_pad);
  lv_obj_set_pos(glow, cx - (w + glow_pad) / 2, cy - (h + glow_pad) / 2);
  lv_obj_set_style_radius(glow, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(glow, color, 0);
  lv_obj_set_style_bg_opa(glow, (lv_opa_t) constrain(glow_base + pulse_shift_ / 4, 6, glow_hi), 0);

  lv_obj_t *mid = lv_obj_create(canvas_);
  lv_obj_remove_style_all(mid);
  lv_obj_set_size(mid, w + mid_pad, h + mid_pad);
  lv_obj_set_pos(mid, cx - (w + mid_pad) / 2, cy - (h + mid_pad) / 2);
  lv_obj_set_style_radius(mid, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(mid, color, 0);
  lv_obj_set_style_bg_opa(mid, (lv_opa_t) constrain(mid_base + pulse_shift_ / 4, 24, mid_hi), 0);

  lv_obj_t *core = lv_obj_create(canvas_);
  lv_obj_remove_style_all(core);
  lv_obj_set_size(core, w, h);
  lv_obj_set_pos(core, cx - w / 2, cy - h / 2);
  lv_obj_set_style_radius(core, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(core, color, 0);
  lv_obj_set_style_bg_opa(core, LV_OPA_COVER, 0);

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

  if (add_side_shadow) {
    lv_obj_t *shadow = lv_obj_create(canvas_);
    lv_obj_remove_style_all(shadow);
    const int16_t sw = (style_ == FACE_STYLE_CLASSIC) ? 12 : 10;
    lv_obj_set_size(shadow, sw, h - 16);
    lv_obj_set_pos(shadow, cx + (isRight ? (w / 2 - sw / 2 + 2) : (-w / 2 - sw / 2 - 2)), cy - (h - 12) / 2);
    lv_obj_set_style_radius(shadow, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(shadow, lv_color_hex(0x1A2538), 0);
    lv_obj_set_style_bg_opa(shadow, (lv_opa_t) ((style_ == FACE_STYLE_CLASSIC) ? 64 : 54), 0);
  }
}
