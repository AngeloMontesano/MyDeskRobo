#include "anime_face.h"
#include <cstring>
#include <stdint.h>
#include <math.h>

int px_sin(uint32_t t, uint32_t period, int amp);

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

  const int16_t eye_gap = 68;
  lv_obj_clean(canvas_);
  drawEye(CX - eye_gap + drift_dx_ + saccade_dx_, CY + drift_dy_ + saccade_dy_, left, false, blink);
  drawEye(CX + eye_gap + drift_dx_ + saccade_dx_, CY + drift_dy_ + saccade_dy_, right, true, blink);
  
  if (style_ == FACE_STYLE_ROUND) {
    drawMouth(CX + drift_dx_ + saccade_dx_, CY + drift_dy_ + saccade_dy_, current_, blink);
  }

  if (current_ == EMOTION_SLEEPY && left == EMOTION_SLEEPY && right == EMOTION_SLEEPY) {
    for(int i = 0; i < 3; i++) {
      int t_offset = now - (i * 500);
      int phase = t_offset % 2000;
      if (phase >= 0 && phase < 1800) {
        lv_obj_t *z_lbl = lv_label_create(canvas_);
        lv_label_set_text(z_lbl, (i == 2) ? "Z" : "z");
        lv_obj_set_style_text_color(z_lbl, lv_color_make(0x0F, 0xDA, 0xFF), 0);
        lv_obj_set_style_text_opa(z_lbl, 255 - (phase * 255 / 1800), 0);
        
        int x_pos = CX + 50 + (i * 20) + (px_sin(phase, 400, 10));
        int y_pos = CY - 30 - (phase / 25) - (i * 10);
        lv_obj_set_pos(z_lbl, x_pos, y_pos);
      }
    }
  }
}

int px_sin(uint32_t t, uint32_t period, int amp) {
  return (int)(sinf((float)(t % period) * 6.2831853f / (float)period) * amp);
}

void AnimeFace::drawFace() {
  const Emotion left = use_pair_override_ ? left_override_ : current_;
  const Emotion right = use_pair_override_ ? right_override_ : current_;
  const int16_t eye_gap = 68;
  drawEye(CX - eye_gap, CY, left, false, false);
  drawEye(CX + eye_gap, CY, right, true, false);

  if (style_ == FACE_STYLE_ROUND) {
    drawMouth(CX, CY, current_, false);
  }
}

void AnimeFace::drawEye(int16_t cx, int16_t cy, Emotion e, bool isRight, bool blink) {
  if (style_ == FACE_STYLE_ROUND) {
    drawEyeRound(cx, cy, e, isRight, blink);
  } else {
    drawEyeEVE(cx, cy, e, isRight, blink);
  }
}

void AnimeFace::drawMouth(int16_t cx, int16_t cy, Emotion e, bool blink) {
  if (blink) return; // simplify mouth on blink

  if (e == EMOTION_SAD) {
      int16_t mw = 20;
      int16_t mh = 20;
      int16_t border = 5;
      lv_obj_t *mouth = lv_obj_create(canvas_);
      lv_obj_remove_style_all(mouth);
      lv_obj_set_size(mouth, mw, mh);
      lv_obj_set_pos(mouth, cx - mw/2, cy + 30);
      lv_obj_set_style_radius(mouth, LV_RADIUS_CIRCLE, 0);
      lv_obj_set_style_border_width(mouth, border, 0);
      lv_obj_set_style_border_color(mouth, lv_color_make(0x0F, 0xDA, 0xFF), 0);
      
      lv_obj_t *cut = lv_obj_create(canvas_);
      lv_obj_remove_style_all(cut);
      lv_obj_set_size(cut, mw + 10, mh/2 + 10);
      lv_obj_set_pos(cut, cx - (mw+10)/2, cy + 30 + mh/2 - border/2 + 2);
      lv_obj_set_style_bg_color(cut, lv_color_hex(0x06080D), 0);
      lv_obj_set_style_bg_opa(cut, LV_OPA_COVER, 0);
  }
  else if (e == EMOTION_HAPPY || e == EMOTION_EXCITED) {
      int16_t mw = 22;
      int16_t mh = 22;
      int16_t border = 5;
      lv_obj_t *mouth = lv_obj_create(canvas_);
      lv_obj_remove_style_all(mouth);
      lv_obj_set_size(mouth, mw, mh);
      lv_obj_set_pos(mouth, cx - mw/2, cy + 20);
      lv_obj_set_style_radius(mouth, LV_RADIUS_CIRCLE, 0);
      lv_obj_set_style_border_width(mouth, border, 0);
      lv_obj_set_style_border_color(mouth, lv_color_make(0x0F, 0xDA, 0xFF), 0);
      
      lv_obj_t *cut = lv_obj_create(canvas_);
      lv_obj_remove_style_all(cut);
      lv_obj_set_size(cut, mw + 10, mh/2 + 10);
      lv_obj_set_pos(cut, cx - (mw+10)/2, cy + 20 - 5);
      lv_obj_set_style_bg_color(cut, lv_color_hex(0x06080D), 0);
      lv_obj_set_style_bg_opa(cut, LV_OPA_COVER, 0);
  }
  else if (e == EMOTION_WOW || e == EMOTION_CONFUSED) {
      int16_t mw = 12;
      int16_t mh = 16;
      lv_obj_t *mouth = lv_obj_create(canvas_);
      lv_obj_remove_style_all(mouth);
      lv_obj_set_size(mouth, mw, mh);
      lv_obj_set_pos(mouth, cx - mw/2 + (e==EMOTION_CONFUSED? 10 : 0), cy + 25);
      lv_obj_set_style_radius(mouth, LV_RADIUS_CIRCLE, 0);
      lv_obj_set_style_border_width(mouth, 4, 0);
      lv_obj_set_style_border_color(mouth, lv_color_make(0x0F, 0xDA, 0xFF), 0);
  }
}

void AnimeFace::drawEyeEVE(int16_t cx, int16_t cy, Emotion e, bool isRight, bool blink) {
  // EVE styling tweaks
  int16_t w = 60; // slightly wider than old eve
  int16_t h = 114; // slightly taller
  lv_color_t color = lv_color_make(0x0F, 0xDA, 0xFF); // slightly brighter cyan
  bool carve_top = false;
  int16_t carve_h = 0;
  int16_t carve_dx = 0;
  int16_t carve_angle = 0;
  bool inner_clip = false;
  int16_t clip_w = 0;
  bool carve_bottom = false;
  int16_t carve_bottom_h = 0;
  bool add_side_shadow = true;
  const lv_color_t bg = lv_color_hex(0x06080D);
  const uint8_t idle_phase = (uint8_t) ((millis() / 2400U) % 4U);

  // Default blink behavior overrides height
  if (blink && (e != EMOTION_WOW)) {
    h = 8;
  }

  switch (e) {
    case EMOTION_HAPPY:
      h = blink ? h : 84;
      carve_bottom = !blink;
      carve_bottom_h = 35;
      cy += 15;
      break;
    case EMOTION_SAD:
      h = blink ? h : 110;
      w = 60;
      cy += 5;
      carve_top = !blink;
      carve_h = 24;       // smaller height for the cut to act like a brow
      carve_dx = isRight ? 10 : -10;
      carve_angle = isRight ? -150 : 150; // Gentle downward curve towards outside for sad
      break;
    case EMOTION_ANGRY:
      h = blink ? h : 110;
      w = 60;
      cy += 5;
      carve_top = !blink;
      carve_h = 24;      // brow cut
      carve_dx = isRight ? -10 : 10;
      carve_angle = isRight ? 200 : -200; // Sharp downward curve towards inside for angry
      break;
    case EMOTION_WOW:
      w = 66;
      h = 114;
      break;
    case EMOTION_SLEEPY:
      h = blink ? h : 16;
      w = 56;
      cy += 35;
      add_side_shadow = false;
      break;
    case EMOTION_DIZZY:
      h = blink ? h : 50;
      w = 50;
      if (!blink) {
        inner_clip = true;
        clip_w = 40;
        cx += isRight ? -10 : 10;
        cy += (millis() / 50) % 20 < 10 ? -5 : 5; // swirly effect
      }
      break;
    case EMOTION_CONFUSED:
      w = 56;
      if (isRight) {
        h = blink ? h : 60;
        cy += 10;
      } else {
        h = blink ? h : 110;
      }
      break;
    case EMOTION_EXCITED:
      w = 68;
      h = blink ? h : 80;
      carve_top = false;
      carve_bottom = !blink;
      carve_bottom_h = 15;
      break;
    case EMOTION_MAIL:
      if (!blink) {
        h = 60;
        w = 90;
        // The drawing logic for the mail envelope is handled at the end of the function,
        // we'll make the core object transparent here.
      } else {
        h = 8;
        w = 90;
      }
      break;
    case EMOTION_CALL:
      if (!blink) {
        h = 80;
        w = 80;
        cx += (int)(sinf(millis() / 30.0f) * 4.0f); // Fast ringing wiggle
        cy += (int)(cosf(millis() / 30.0f) * 4.0f);
        // Drawing logic for phone handled at the end of the function.
      } else {
        h = 8;
        w = 60;
      }
      break;
    case EMOTION_SHAKE:
      h = blink ? h : 110;
      w = 60;
      // High speed bouncing
      cx += (int)(sinf(millis() / 50.0f) * 80.0f);
      cy += (int)(cosf(millis() / 70.0f) * 80.0f);
      break;
    case EMOTION_IDLE:
    default:
      if (idle_phase == 0) {
        h = blink ? 8 : 114;
        w = 56;
        cy += (int)(sinf(millis() / 500.0f) * 6.0f); // gentle vertical breathing
      } else if (idle_phase == 1) {
        h = blink ? 8 : 106;
        w = 60;
        cy += 3;
        carve_top = !blink;
        carve_h = 10;
        cx += (int)(sinf(millis() / 800.0f) * 16.0f); // slow side look
      } else if (idle_phase == 2) {
        h = blink ? 8 : 98;
        w = 64;
        cy += 10;
        add_side_shadow = false;
        cx += (int)(sinf(millis() / 400.0f) * 12.0f); // circular rolling
        cy -= (int)(cosf(millis() / 400.0f) * 12.0f);
      } else {
        h = blink ? 8 : 110;
        w = 58;
        cx -= (int)(sinf(millis() / 700.0f) * 10.0f); // gentle sway
      }
      break;
  }
  if (w < 10) w = 10;
  if (h < 6) h = 6;

  const int glow_pad = 8;
  const int mid_pad = 3;
  const int glow_base = 20;
  const int glow_hi = 60;
  const int mid_base = 86;
  const int mid_hi = 140;
  const lv_coord_t eye_radius = LV_RADIUS_CIRCLE;

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

  if ((e == EMOTION_MAIL || e == EMOTION_CALL) && !blink) {
     // Don't draw the regular eye core for these custom shapes
     lv_obj_set_style_bg_opa(core, LV_OPA_TRANSP, 0);
     lv_obj_set_style_bg_opa(glow, LV_OPA_TRANSP, 0);
     lv_obj_set_style_bg_opa(mid, LV_OPA_TRANSP, 0);
     add_side_shadow = false;
  } else {
     lv_obj_set_style_bg_color(core, color, 0);
     lv_obj_set_style_bg_opa(core, LV_OPA_COVER, 0);
  }

  if (carve_top && (carve_h > 0)) {
    lv_obj_t *cut = lv_obj_create(canvas_);
    lv_obj_remove_style_all(cut);
    lv_obj_set_size(cut, w + 30, carve_h + 30);
    lv_obj_set_pos(cut, cx - (w + 30) / 2 + carve_dx, cy - h / 2 - 20);
    lv_obj_set_style_bg_color(cut, bg, 0);
    lv_obj_set_style_bg_opa(cut, LV_OPA_COVER, 0);
    if (carve_angle != 0) lv_obj_set_style_transform_angle(cut, carve_angle, 0);
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
    lv_obj_set_size(cut, w + 20, carve_bottom_h);
    lv_obj_set_pos(cut, cx - (w + 20) / 2, cy + h / 2 - carve_bottom_h + 2);
    lv_obj_set_style_bg_color(cut, bg, 0);
    lv_obj_set_style_bg_opa(cut, LV_OPA_COVER, 0);
  }

  if (add_side_shadow) {
    lv_obj_t *shadow = lv_obj_create(canvas_);
    lv_obj_remove_style_all(shadow);
    const int16_t sw = 10;
    lv_obj_set_size(shadow, sw, h - 16);
    lv_obj_set_pos(shadow, cx + (isRight ? (w / 2 - sw / 2 + 2) : (-w / 2 - sw / 2 - 2)), cy - (h - 12) / 2);
    lv_obj_set_style_radius(shadow, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(shadow, lv_color_hex(0x1A2538), 0);
    lv_obj_set_style_bg_opa(shadow, (lv_opa_t) 54, 0);
  }

  if (e == EMOTION_MAIL && !blink) {
    // Draw an envelope explicitly using multiple objects
    lv_obj_t *body = lv_obj_create(canvas_);
    lv_obj_remove_style_all(body);
    lv_obj_set_size(body, w, h);
    lv_obj_set_pos(body, cx - w/2, cy - h/2);
    lv_obj_set_style_radius(body, 6, 0);
    lv_obj_set_style_bg_color(body, bg, 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(body, color, 0);
    lv_obj_set_style_border_width(body, 4, 0);

    lv_obj_t *flap = lv_obj_create(canvas_);
    lv_obj_remove_style_all(flap);
    lv_obj_set_size(flap, w - 8, h/2 + 2);
    lv_obj_set_pos(flap, cx - (w-8)/2, cy - h/2 - 2);
    lv_obj_set_style_bg_color(flap, bg, 0);
    lv_obj_set_style_bg_opa(flap, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(flap, color, 0);
    lv_obj_set_style_border_width(flap, 4, 0);
    lv_obj_set_style_transform_angle(flap, 0, 0);
    lv_obj_set_style_radius(flap, 4, 0);

    // Cover the top edge of the flap so it looks like a triangle coming down
    lv_obj_t *flap_mask = lv_obj_create(canvas_);
    lv_obj_remove_style_all(flap_mask);
    lv_obj_set_size(flap_mask, w + 10, h/2);
    lv_obj_set_pos(flap_mask, cx - (w+10)/2, cy - h/2 - h/4);
    lv_obj_set_style_bg_color(flap_mask, bg, 0);
    lv_obj_set_style_bg_opa(flap_mask, LV_OPA_COVER, 0);
    lv_obj_set_style_transform_angle(flap_mask, isRight ? 200 : -200, 0);
  }

  if (e == EMOTION_CALL && !blink) {
    // Draw a phone handset explicitly
    lv_obj_t *handle = lv_obj_create(canvas_);
    lv_obj_remove_style_all(handle);
    lv_obj_set_size(handle, 16, h - 20);
    lv_obj_set_pos(handle, cx - 8, cy - (h-20)/2);
    lv_obj_set_style_radius(handle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(handle, color, 0);
    lv_obj_set_style_bg_opa(handle, LV_OPA_COVER, 0);

    lv_obj_t *top_speaker = lv_obj_create(canvas_);
    lv_obj_remove_style_all(top_speaker);
    lv_obj_set_size(top_speaker, 36, 16);
    lv_obj_set_pos(top_speaker, cx - 18, cy - h/2 + 4);
    lv_obj_set_style_radius(top_speaker, 8, 0);
    lv_obj_set_style_bg_color(top_speaker, color, 0);
    lv_obj_set_style_bg_opa(top_speaker, LV_OPA_COVER, 0);
    lv_obj_set_style_transform_angle(top_speaker, 200, 0);

    lv_obj_t *bot_speaker = lv_obj_create(canvas_);
    lv_obj_remove_style_all(bot_speaker);
    lv_obj_set_size(bot_speaker, 36, 16);
    lv_obj_set_pos(bot_speaker, cx - 18, cy + h/2 - 20);
    lv_obj_set_style_radius(bot_speaker, 8, 0);
    lv_obj_set_style_bg_color(bot_speaker, color, 0);
    lv_obj_set_style_bg_opa(bot_speaker, LV_OPA_COVER, 0);
    lv_obj_set_style_transform_angle(bot_speaker, -200, 0);
  }
}

void AnimeFace::drawEyeRound(int16_t cx, int16_t cy, Emotion e, bool isRight, bool blink) {
  int16_t w = 44;
  int16_t h = 44;
  lv_color_t color = lv_color_make(0x0F, 0xDA, 0xFF);
  bool carve_top = false;
  int16_t carve_h = 0;
  int16_t carve_dx = 0;
  int16_t carve_angle = 0;
  bool carve_bottom = false;
  int16_t carve_bottom_h = 0;
  bool carve_bottom_curve = false;
  const lv_color_t bg = lv_color_hex(0x06080D);

  if (blink && (e != EMOTION_WOW)) {
    h = 8;
  }

  switch (e) {
    case EMOTION_HAPPY:
      h = blink ? h : 44;
      carve_bottom = !blink;
      carve_bottom_h = 24;
      cy += 5;
      break;
    case EMOTION_SAD:
      h = blink ? h : 44;
      cy += 5;
      if (!blink) {
        carve_bottom_curve = true;
      }
      break;
    case EMOTION_ANGRY:
      h = blink ? h : 44;
      cy -= 2;
      carve_top = !blink;
      carve_h = 24;
      carve_angle = isRight ? 160 : -160;
      carve_dx = isRight ? -4 : 4;
      break;
    case EMOTION_WOW:
      w = 50; h = 50;
      break;
    case EMOTION_SLEEPY:
      h = blink ? h : 10;
      cy += 15;
      break;
    case EMOTION_CONFUSED:
      if (isRight) {
        w = 34; h = blink ? h : 34;
        cy += 5;
      }
      break;
    case EMOTION_EXCITED:
      w = 50; h = blink ? h : 50;
      break;
    case EMOTION_DIZZY:
      w = 30; h = blink ? h : 30;
      if (!blink) {
        cx += (millis() / 50) % 20 < 10 ? -4 : 4;
        cy += (millis() / 50) % 20 < 10 ? -4 : 4;
      }
      break;
    case EMOTION_IDLE:
    default:
      h = blink ? 8 : 44;
      break;
  }

  if (w < 6) w = 6;
  if (h < 6) h = 6;

  const int glow_pad = 12;
  const lv_coord_t eye_radius = LV_RADIUS_CIRCLE;

  lv_obj_t *glow = lv_obj_create(canvas_);
  lv_obj_remove_style_all(glow);
  lv_obj_set_size(glow, w + glow_pad, h + glow_pad);
  lv_obj_set_pos(glow, cx - (w + glow_pad) / 2, cy - (h + glow_pad) / 2);
  lv_obj_set_style_radius(glow, eye_radius, 0);
  lv_obj_set_style_bg_color(glow, color, 0);
  lv_obj_set_style_bg_opa(glow, (lv_opa_t) constrain(40 + pulse_shift_ / 3, 10, 80), 0);

  lv_obj_t *core = lv_obj_create(canvas_);
  lv_obj_remove_style_all(core);
  lv_obj_set_size(core, w, h);
  lv_obj_set_pos(core, cx - w / 2, cy - h / 2);
  lv_obj_set_style_radius(core, eye_radius, 0);
  lv_obj_set_style_bg_color(core, color, 0);
  lv_obj_set_style_bg_opa(core, LV_OPA_COVER, 0);

  if (carve_top && (carve_h > 0)) {
    lv_obj_t *cut = lv_obj_create(canvas_);
    lv_obj_remove_style_all(cut);
    lv_obj_set_size(cut, w + 24, carve_h + 24);
    lv_obj_set_pos(cut, cx - (w + 24) / 2 + carve_dx, cy - h / 2 - 12);
    lv_obj_set_style_bg_color(cut, bg, 0);
    lv_obj_set_style_bg_opa(cut, LV_OPA_COVER, 0);
    if (carve_angle != 0) lv_obj_set_style_transform_angle(cut, carve_angle, 0);
  }

  if (carve_bottom && (carve_bottom_h > 0)) {
    lv_obj_t *cut = lv_obj_create(canvas_);
    lv_obj_remove_style_all(cut);
    lv_obj_set_size(cut, w + 20, carve_bottom_h);
    lv_obj_set_pos(cut, cx - (w + 20) / 2, cy + h / 2 - carve_bottom_h + 2);
    lv_obj_set_style_bg_color(cut, bg, 0);
    lv_obj_set_style_bg_opa(cut, LV_OPA_COVER, 0);
  }

  if (carve_bottom_curve) {
    lv_obj_t *cut = lv_obj_create(canvas_);
    lv_obj_remove_style_all(cut);
    int16_t cw = w + 24;
    lv_obj_set_size(cut, cw, cw);
    lv_obj_set_pos(cut, cx - cw/2 + (isRight ? 6 : -6), cy + h/2 - 16);
    lv_obj_set_style_radius(cut, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(cut, bg, 0);
    lv_obj_set_style_bg_opa(cut, LV_OPA_COVER, 0);
  }
}
