#include "anime_face.h"
#include <cstring>
#include <stdint.h>
#include <math.h>

int px_sin(uint32_t t, uint32_t period, int amp);

static lv_color_t tuned_eye_color(const AnimeFace::Tuning &tuning) {
  return lv_color_make((uint8_t)constrain(tuning.eye_color_r, 0, 255),
                       (uint8_t)constrain(tuning.eye_color_g, 0, 255),
                       (uint8_t)constrain(tuning.eye_color_b, 0, 255));
}

static lv_color_t tuned_eye_glow(const AnimeFace::Tuning &tuning, uint8_t boost) {
  const int r = constrain(tuning.eye_color_r + boost, 0, 255);
  const int g = constrain(tuning.eye_color_g + boost, 0, 255);
  const int b = constrain(tuning.eye_color_b + boost, 0, 255);
  return lv_color_make((uint8_t)r, (uint8_t)g, (uint8_t)b);
}


static bool is_angry_lab_style(FaceStyle style) {
  return style == FACE_STYLE_ANGRY_LAB;
}

static bool is_sad_lab_style(FaceStyle style) {
  return style == FACE_STYLE_SAD_LAB;
}

static int mood_lab_variant_index(Emotion e) {
  if (e >= EMOTION_ANGRY_1 && e <= EMOTION_ANGRY_10) return (int)e - (int)EMOTION_ANGRY_1;
  if (e >= EMOTION_SAD_1 && e <= EMOTION_SAD_10) return (int)e - (int)EMOTION_SAD_1;
  return -1;
}

struct MoodVariant {
  int16_t eye_w;
  int16_t eye_h;
  int16_t top_cut_h;
  int16_t brow_w;
  int16_t brow_h;
  int16_t brow_gap;
  int16_t brow_angle;
  int16_t brow_dx;
  int16_t eye_y;
};

static const MoodVariant kAngryVariants[10] = {
    {82, 84, 4, 54, 8, 16, 180, 2, 2},
    {64, 74, 10, 34, 6, 13, 320, 0, 5},
    {56, 60, 18, 30, 6, 10, 520, 1, 8},
    {76, 88, 8, 58, 9, 15, 620, 8, 2},
    {52, 50, 26, 40, 8, 18, 780, 2, 11},
    {70, 62, 20, 48, 8, 10, 900, 6, 7},
    {60, 80, 6, 40, 7, 18, 260, 8, 3},
    {68, 48, 30, 44, 8, 6, 1040, 3, 13},
    {58, 68, 14, 36, 7, 12, 420, 5, 7},
    {84, 56, 24, 62, 10, 7, 1180, 10, 10},
};

static const MoodVariant kSadVariants[10] = {
    {68, 108, 0, 50, 7, 11, 140, 7, 7},
    {52, 98, 0, 28, 5, 8, 240, 0, 10},
    {74, 86, 0, 56, 8, 14, 340, 9, 12},
    {48, 76, 0, 22, 5, 15, 460, 1, 17},
    {78, 94, 0, 58, 8, 9, 560, 10, 10},
    {56, 68, 0, 30, 5, 16, 680, 0, 19},
    {62, 114, 0, 36, 6, 7, 180, 4, 6},
    {50, 84, 0, 24, 5, 13, 800, 2, 15},
    {72, 74, 0, 48, 8, 12, 920, 10, 14},
    {46, 102, 0, 20, 5, 8, 280, 1, 8},
};

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
      style_(FACE_STYLE_EVE_CINEMATIC) {
  tuning_.drift_amp_px = 2;
  tuning_.saccade_amp_px = 4;
  tuning_.saccade_min_ms = 900;
  tuning_.saccade_max_ms = 2200;
  tuning_.blink_interval_ms = 3900;
  tuning_.blink_duration_ms = 120;
  tuning_.double_blink_chance_pct = 24;
  tuning_.glow_pulse_amp = 20;
  tuning_.glow_pulse_period_ms = 1800;
  tuning_.shake_amp_px = 24;
  tuning_.shake_period_ms = 700;
  tuning_.eye_color_r = 15;
  tuning_.eye_color_g = 218;
  tuning_.eye_color_b = 255;
}

void AnimeFace::init(lv_obj_t *parent) {
  parent_ = parent;
  canvas_ = lv_obj_create(parent);
  lv_obj_remove_style_all(canvas_);
  lv_obj_clear_flag(canvas_, LV_OBJ_FLAG_SCROLLABLE);
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
  else if (strcmp(key, "shake_amp_px") == 0) tuning_.shake_amp_px = constrain(value, 4, 48);
  else if (strcmp(key, "shake_period_ms") == 0) tuning_.shake_period_ms = constrain(value, 240, 2200);
  else if (strcmp(key, "eye_color_r") == 0) tuning_.eye_color_r = constrain(value, 0, 255);
  else if (strcmp(key, "eye_color_g") == 0) tuning_.eye_color_g = constrain(value, 0, 255);
  else if (strcmp(key, "eye_color_b") == 0) tuning_.eye_color_b = constrain(value, 0, 255);
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
  if (strcmp(key, "shake_amp_px") == 0) return tuning_.shake_amp_px;
  if (strcmp(key, "shake_period_ms") == 0) return tuning_.shake_period_ms;
  if (strcmp(key, "eye_color_r") == 0) return tuning_.eye_color_r;
  if (strcmp(key, "eye_color_g") == 0) return tuning_.eye_color_g;
  if (strcmp(key, "eye_color_b") == 0) return tuning_.eye_color_b;
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
  out += ",\"shake_amp_px\":" + String(tuning_.shake_amp_px);
  out += ",\"shake_period_ms\":" + String(tuning_.shake_period_ms);
  out += ",\"eye_color_r\":" + String(tuning_.eye_color_r);
  out += ",\"eye_color_g\":" + String(tuning_.eye_color_g);
  out += ",\"eye_color_b\":" + String(tuning_.eye_color_b);
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
  

  if (current_ == EMOTION_SLEEPY || left == EMOTION_SLEEPY || right == EMOTION_SLEEPY) {
    for (int i = 0; i < 3; i++) {
      const uint32_t phase = (now + (uint32_t)i * 680U) % 2400U;
      if (phase < 2000U) {
        lv_obj_t *z_lbl = lv_label_create(canvas_);
        lv_label_set_text(z_lbl, (i == 0) ? "ZZ" : "Z");
        lv_obj_set_style_text_font(z_lbl, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(z_lbl, lv_color_make(0x0F, 0xDA, 0xFF), 0);
        lv_obj_set_style_text_opa(z_lbl, (lv_opa_t)(255 - (phase * 255U / 2000U)), 0);

        const int x_pos = CX + 50 + (i * 22) + px_sin(phase, 460, 10);
        const int y_pos = CY - 42 - (int)(phase / 26U) - (i * 14);
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

}

void AnimeFace::drawEye(int16_t cx, int16_t cy, Emotion e, bool isRight, bool blink) {
  if (style_ == FACE_STYLE_FLUX) {
    drawEyeFlux(cx, cy, e, isRight, blink);
  } else if (is_angry_lab_style(style_) || is_sad_lab_style(style_)) {
    drawEyeMoodLab(cx, cy, e, isRight, blink);
  } else {
    drawEyeEVE(cx, cy, e, isRight, blink);
  }
}

void AnimeFace::drawEyeEVE(int16_t cx, int16_t cy, Emotion e, bool isRight, bool blink) {
  const bool subtle_style = false;
  const bool comic_style = false;

  int16_t w = 60;
  int16_t h = 114;
  lv_color_t color = tuned_eye_color(tuning_);
  bool carve_top = false;
  int16_t carve_h = 0;
  int16_t carve_dx = 0;
  int16_t carve_angle = 0;
  bool inner_clip = false;
  int16_t clip_w = 0;
  bool carve_bottom = false;
  int16_t carve_bottom_h = 0;
  bool add_side_shadow = true;
  bool add_brow_angry = false;
  bool add_brow_sad = false;
  int16_t brow_w = 0;
  int16_t brow_h = 0;
  int16_t brow_gap = 0;
  int16_t brow_angle = 0;
  int16_t brow_dx = 0;
  const lv_color_t bg = lv_color_hex(0x06080D);
  const uint8_t idle_phase = (uint8_t)((millis() / 2400U) % 4U);

  if (blink && (e != EMOTION_WOW)) {
    h = 8;
  }

  switch (e) {
    case EMOTION_HAPPY:
      if (subtle_style) {
        h = blink ? h : 80;
        w = 54;
        carve_bottom = !blink;
        carve_bottom_h = 30;
        cy += 12;
      } else if (comic_style) {
        h = blink ? h : 92;
        w = 70;
        carve_bottom = !blink;
        carve_bottom_h = 44;
        cy += 18;
      } else {
        h = blink ? h : 84;
        carve_bottom = !blink;
        carve_bottom_h = 35;
        cy += 15;
      }
      break;
    case EMOTION_SAD:
      if (subtle_style) {
        h = blink ? h : 84;
        w = 56;
        cy += 10;
        add_side_shadow = false;
        add_brow_sad = !blink;
        brow_w = 34;
        brow_h = 5;
        brow_gap = 12;
        brow_angle = 260;
        brow_dx = 3;
      } else if (comic_style) {
        h = blink ? h : 94;
        w = 66;
        cy += 12;
        add_brow_sad = !blink;
        brow_w = 48;
        brow_h = 7;
        brow_gap = 13;
        brow_angle = 340;
        brow_dx = 5;
      } else {
        h = blink ? h : 88;
        w = 60;
        cy += 11;
        add_side_shadow = false;
        add_brow_sad = !blink;
        brow_w = 42;
        brow_h = 6;
        brow_gap = 13;
        brow_angle = 300;
        brow_dx = 4;
      }
      break;
    case EMOTION_ANGRY:
      if (subtle_style) {
        h = blink ? h : 62;
        w = 58;
        cy += 8;
        carve_top = !blink;
        carve_h = 8;
        add_brow_angry = !blink;
        brow_w = 36;
        brow_h = 5;
        brow_gap = 12;
        brow_angle = 520;
        brow_dx = 4;
      } else if (comic_style) {
        h = blink ? h : 72;
        w = 68;
        cy += 8;
        carve_top = !blink;
        carve_h = 14;
        add_brow_angry = !blink;
        brow_w = 52;
        brow_h = 7;
        brow_gap = 14;
        brow_angle = 760;
        brow_dx = 6;
      } else {
        h = blink ? h : 66;
        w = 62;
        cy += 8;
        carve_top = !blink;
        carve_h = 10;
        add_brow_angry = !blink;
        brow_w = 46;
        brow_h = 6;
        brow_gap = 13;
        brow_angle = 620;
        brow_dx = 5;
      }
      break;
    case EMOTION_WOW:
      w = subtle_style ? 62 : (comic_style ? 74 : 66);
      h = subtle_style ? 108 : (comic_style ? 124 : 114);
      break;
    case EMOTION_SLEEPY:
      h = blink ? h : (subtle_style ? 14 : (comic_style ? 18 : 16));
      w = subtle_style ? 52 : (comic_style ? 60 : 56);
      cy += 35;
      add_side_shadow = false;
      break;
    case EMOTION_DIZZY:
      h = blink ? h : (subtle_style ? 44 : (comic_style ? 58 : 50));
      w = subtle_style ? 46 : (comic_style ? 56 : 50);
      if (!blink) {
        inner_clip = true;
        clip_w = subtle_style ? 36 : (comic_style ? 46 : 40);
        cx += isRight ? -10 : 10;
        cy += (millis() / 50) % 20 < 10 ? -5 : 5;
      }
      break;
    case EMOTION_CONFUSED:
      w = subtle_style ? 54 : (comic_style ? 62 : 56);
      if (isRight) {
        h = blink ? h : (subtle_style ? 56 : (comic_style ? 66 : 60));
        cy += 10;
      } else {
        h = blink ? h : (subtle_style ? 104 : (comic_style ? 116 : 110));
      }
      break;
    case EMOTION_EXCITED:
      w = subtle_style ? 62 : (comic_style ? 76 : 68);
      h = blink ? h : (subtle_style ? 74 : (comic_style ? 88 : 80));
      carve_bottom = !blink;
      carve_bottom_h = subtle_style ? 12 : (comic_style ? 22 : 15);
      break;
    case EMOTION_MAIL:
      if (!blink) {
        h = subtle_style ? 54 : (comic_style ? 68 : 60);
        w = subtle_style ? 84 : (comic_style ? 100 : 90);
      } else {
        h = 8;
        w = subtle_style ? 84 : (comic_style ? 100 : 90);
      }
      break;
    case EMOTION_CALL:
      if (!blink) {
        h = subtle_style ? 74 : (comic_style ? 88 : 80);
        w = subtle_style ? 74 : (comic_style ? 88 : 80);
        const float wiggle = subtle_style ? 3.0f : (comic_style ? 6.0f : 4.0f);
        cx += (int)(sinf(millis() / 30.0f) * wiggle);
        cy += (int)(cosf(millis() / 30.0f) * wiggle);
      } else {
        h = 8;
        w = subtle_style ? 56 : (comic_style ? 70 : 60);
      }
      break;
    case EMOTION_SHAKE: {
      const int amp = tuning_.shake_amp_px + (subtle_style ? -4 : (comic_style ? 8 : 0));
      const int period = tuning_.shake_period_ms + (subtle_style ? 80 : (comic_style ? -80 : 0));
      const float phase = (6.2831853f * (float)(millis() % (uint32_t)period) / (float)period) + (isRight ? 1.5707963f : -1.5707963f);
      const float amp_y = (float)amp * 0.55f;
      h = blink ? h : (subtle_style ? 96 : (comic_style ? 108 : 100));
      w = subtle_style ? 58 : (comic_style ? 68 : 62);
      cx += (int)(sinf(phase) * (float)amp);
      cy += (int)(fabsf(sinf(phase * 1.15f)) * amp_y) - (int)(amp_y * 0.55f);
      break;
    }
    case EMOTION_IDLE:
    default:
      if (idle_phase == 0) {
        if (subtle_style) {
          h = blink ? 8 : 106;
          w = 54;
          cy += (int)(sinf(millis() / 700.0f) * 4.0f);
        } else if (comic_style) {
          h = blink ? 8 : 118;
          w = 66;
          cy += (int)(sinf(millis() / 420.0f) * 9.0f);
        } else {
          h = blink ? 8 : 114;
          w = 56;
          cy += (int)(sinf(millis() / 500.0f) * 6.0f);
        }
      } else if (idle_phase == 1) {
        if (subtle_style) {
          h = blink ? 8 : 100;
          w = 56;
          cy += 2;
          carve_top = !blink;
          carve_h = 8;
          cx += (int)(sinf(millis() / 1000.0f) * 10.0f);
        } else if (comic_style) {
          h = blink ? 8 : 110;
          w = 68;
          cy += 4;
          carve_top = !blink;
          carve_h = 14;
          cx += (int)(sinf(millis() / 600.0f) * 22.0f);
        } else {
          h = blink ? 8 : 106;
          w = 60;
          cy += 3;
          carve_top = !blink;
          carve_h = 10;
          cx += (int)(sinf(millis() / 800.0f) * 16.0f);
        }
      } else if (idle_phase == 2) {
        if (subtle_style) {
          h = blink ? 8 : 92;
          w = 58;
          cy += 8;
          add_side_shadow = false;
          cx += (int)(sinf(millis() / 520.0f) * 8.0f);
          cy -= (int)(cosf(millis() / 520.0f) * 8.0f);
        } else if (comic_style) {
          h = blink ? 8 : 102;
          w = 70;
          cy += 12;
          cx += (int)(sinf(millis() / 320.0f) * 16.0f);
          cy -= (int)(cosf(millis() / 320.0f) * 16.0f);
        } else {
          h = blink ? 8 : 98;
          w = 64;
          cy += 10;
          add_side_shadow = false;
          cx += (int)(sinf(millis() / 400.0f) * 12.0f);
          cy -= (int)(cosf(millis() / 400.0f) * 12.0f);
        }
      } else {
        if (subtle_style) {
          h = blink ? 8 : 104;
          w = 55;
          cx -= (int)(sinf(millis() / 900.0f) * 7.0f);
        } else if (comic_style) {
          h = blink ? 8 : 114;
          w = 64;
          cx -= (int)(sinf(millis() / 520.0f) * 14.0f);
        } else {
          h = blink ? 8 : 110;
          w = 58;
          cx -= (int)(sinf(millis() / 700.0f) * 10.0f);
        }
      }
      break;
  }

  if (w < 10) w = 10;
  if (h < 6) h = 6;

  const int glow_pad = subtle_style ? 6 : (comic_style ? 10 : 8);
  const int mid_pad = subtle_style ? 2 : (comic_style ? 4 : 3);
  const int glow_base = subtle_style ? 14 : (comic_style ? 26 : 20);
  const int glow_hi = subtle_style ? 46 : (comic_style ? 70 : 60);
  const int mid_base = subtle_style ? 70 : (comic_style ? 96 : 86);
  const int mid_hi = subtle_style ? 122 : (comic_style ? 156 : 140);
  const lv_coord_t eye_radius = LV_RADIUS_CIRCLE;

  lv_obj_t *glow = lv_obj_create(canvas_);
  lv_obj_remove_style_all(glow);
  lv_obj_set_size(glow, w + glow_pad, h + glow_pad);
  lv_obj_set_pos(glow, cx - (w + glow_pad) / 2, cy - (h + glow_pad) / 2);
  lv_obj_set_style_radius(glow, eye_radius, 0);
  lv_obj_set_style_bg_color(glow, color, 0);
  lv_obj_set_style_bg_opa(glow, (lv_opa_t)constrain(glow_base + pulse_shift_ / 4, 6, glow_hi), 0);

  lv_obj_t *mid = lv_obj_create(canvas_);
  lv_obj_remove_style_all(mid);
  lv_obj_set_size(mid, w + mid_pad, h + mid_pad);
  lv_obj_set_pos(mid, cx - (w + mid_pad) / 2, cy - (h + mid_pad) / 2);
  lv_obj_set_style_radius(mid, eye_radius, 0);
  lv_obj_set_style_bg_color(mid, color, 0);
  lv_obj_set_style_bg_opa(mid, (lv_opa_t)constrain(mid_base + pulse_shift_ / 4, 24, mid_hi), 0);

  lv_obj_t *core = lv_obj_create(canvas_);
  lv_obj_remove_style_all(core);
  lv_obj_set_size(core, w, h);
  lv_obj_set_pos(core, cx - w / 2, cy - h / 2);
  lv_obj_set_style_radius(core, eye_radius, 0);

  if ((e == EMOTION_MAIL || e == EMOTION_CALL) && !blink) {
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
    const int16_t sw = subtle_style ? 8 : (comic_style ? 12 : 10);
    lv_obj_set_size(shadow, sw, h - 16);
    lv_obj_set_pos(shadow, cx + (isRight ? (w / 2 - sw / 2 + 2) : (-w / 2 - sw / 2 - 2)), cy - (h - 12) / 2);
    lv_obj_set_style_radius(shadow, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(shadow, lv_color_hex(0x1A2538), 0);
    lv_obj_set_style_bg_opa(shadow, (lv_opa_t)54, 0);
  }

  if (add_brow_angry) {
    lv_obj_t *brow = lv_obj_create(canvas_);
    lv_obj_remove_style_all(brow);
    lv_obj_set_size(brow, brow_w, brow_h);
    lv_obj_set_pos(brow, cx - brow_w / 2 + (isRight ? brow_dx : -brow_dx), cy - h / 2 - brow_gap);
    lv_obj_set_style_radius(brow, 2, 0);
    lv_obj_set_style_bg_color(brow, color, 0);
    lv_obj_set_style_bg_opa(brow, LV_OPA_COVER, 0);
    lv_obj_set_style_transform_angle(brow, isRight ? brow_angle : -brow_angle, 0);
  }

  if (add_brow_sad) {
    lv_obj_t *brow = lv_obj_create(canvas_);
    lv_obj_remove_style_all(brow);
    lv_obj_set_size(brow, brow_w, brow_h);
    lv_obj_set_pos(brow, cx - brow_w / 2 + (isRight ? brow_dx : -brow_dx), cy - h / 2 - brow_gap);
    lv_obj_set_style_radius(brow, 2, 0);
    lv_obj_set_style_bg_color(brow, color, 0);
    lv_obj_set_style_bg_opa(brow, LV_OPA_COVER, 0);
    lv_obj_set_style_transform_angle(brow, isRight ? -brow_angle : brow_angle, 0);
  }

  if (e == EMOTION_MAIL && !blink) {
    lv_obj_t *body = lv_obj_create(canvas_);
    lv_obj_remove_style_all(body);
    lv_obj_set_size(body, w, h);
    lv_obj_set_pos(body, cx - w / 2, cy - h / 2);
    lv_obj_set_style_radius(body, 6, 0);
    lv_obj_set_style_bg_color(body, bg, 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(body, color, 0);
    lv_obj_set_style_border_width(body, 4, 0);

    lv_obj_t *flap = lv_obj_create(canvas_);
    lv_obj_remove_style_all(flap);
    lv_obj_set_size(flap, w - 8, h / 2 + 2);
    lv_obj_set_pos(flap, cx - (w - 8) / 2, cy - h / 2 - 2);
    lv_obj_set_style_bg_color(flap, bg, 0);
    lv_obj_set_style_bg_opa(flap, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(flap, color, 0);
    lv_obj_set_style_border_width(flap, 4, 0);
    lv_obj_set_style_transform_angle(flap, 0, 0);
    lv_obj_set_style_radius(flap, 4, 0);

    lv_obj_t *flap_mask = lv_obj_create(canvas_);
    lv_obj_remove_style_all(flap_mask);
    lv_obj_set_size(flap_mask, w + 10, h / 2);
    lv_obj_set_pos(flap_mask, cx - (w + 10) / 2, cy - h / 2 - h / 4);
    lv_obj_set_style_bg_color(flap_mask, bg, 0);
    lv_obj_set_style_bg_opa(flap_mask, LV_OPA_COVER, 0);
    lv_obj_set_style_transform_angle(flap_mask, isRight ? 200 : -200, 0);
  }

  if (e == EMOTION_CALL && !blink) {
    lv_obj_t *handle = lv_obj_create(canvas_);
    lv_obj_remove_style_all(handle);
    lv_obj_set_size(handle, 16, h - 20);
    lv_obj_set_pos(handle, cx - 8, cy - (h - 20) / 2);
    lv_obj_set_style_radius(handle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(handle, color, 0);
    lv_obj_set_style_bg_opa(handle, LV_OPA_COVER, 0);

    lv_obj_t *top_speaker = lv_obj_create(canvas_);
    lv_obj_remove_style_all(top_speaker);
    lv_obj_set_size(top_speaker, 36, 16);
    lv_obj_set_pos(top_speaker, cx - 18, cy - h / 2 + 4);
    lv_obj_set_style_radius(top_speaker, 8, 0);
    lv_obj_set_style_bg_color(top_speaker, color, 0);
    lv_obj_set_style_bg_opa(top_speaker, LV_OPA_COVER, 0);
    lv_obj_set_style_transform_angle(top_speaker, 200, 0);

    lv_obj_t *bot_speaker = lv_obj_create(canvas_);
    lv_obj_remove_style_all(bot_speaker);
    lv_obj_set_size(bot_speaker, 36, 16);
    lv_obj_set_pos(bot_speaker, cx - 18, cy + h / 2 - 20);
    lv_obj_set_style_radius(bot_speaker, 8, 0);
    lv_obj_set_style_bg_color(bot_speaker, color, 0);
    lv_obj_set_style_bg_opa(bot_speaker, LV_OPA_COVER, 0);
    lv_obj_set_style_transform_angle(bot_speaker, -200, 0);
  }
}

void AnimeFace::drawEyeFlux(int16_t cx, int16_t cy, Emotion e, bool isRight, bool blink) {
  const lv_color_t color = tuned_eye_color(tuning_);
  const lv_color_t glow_color = tuned_eye_glow(tuning_, 40);
  const lv_color_t bg = lv_color_hex(0x06080D);
  int16_t w = 76;
  int16_t h = 42;
  int16_t angle = 0;
  bool hollow = false;
  bool add_brow = false;

  if (blink) {
    h = 8;
  }

  switch (e) {
    case EMOTION_HAPPY:
      if (!blink) {
        h = 30;
        cy += 8;
        angle = isRight ? -80 : 80;
      }
      break;
    case EMOTION_SAD:
      if (!blink) {
        h = 26;
        cy += 14;
        angle = isRight ? 90 : -90;
      }
      break;
    case EMOTION_ANGRY:
      if (!blink) {
        h = 28;
        cy += 2;
        angle = isRight ? -120 : 120;
        add_brow = true;
      }
      break;
    case EMOTION_WOW:
      w = 54;
      h = blink ? h : 72;
      break;
    case EMOTION_SLEEPY:
      w = 80;
      h = blink ? h : 10;
      cy += 30;
      break;
    case EMOTION_CONFUSED:
      if (isRight) {
        w = 62;
        h = blink ? h : 24;
        cy += 12;
      } else {
        w = 72;
        h = blink ? h : 50;
        cy += 2;
      }
      break;
    case EMOTION_EXCITED:
      w = 82;
      h = blink ? h : 36;
      cy += 4;
      break;
    case EMOTION_DIZZY:
      w = 50;
      h = blink ? h : 50;
      if (!blink) {
        cx += isRight ? -8 : 8;
        cy += ((millis() / 90U) % 2U) ? -4 : 4;
      }
      break;
    case EMOTION_MAIL:
      w = 82;
      h = blink ? 8 : 48;
      hollow = !blink;
      break;
    case EMOTION_CALL:
      w = 62;
      h = blink ? 8 : 62;
      hollow = !blink;
      if (!blink) {
        cx += (int)(sinf(millis() / 80.0f) * 3.0f);
      }
      break;
    case EMOTION_SHAKE: {
      const int amp = tuning_.shake_amp_px;
      const int period = tuning_.shake_period_ms;
      const float phase = (6.2831853f * (float)(millis() % (uint32_t)period) / (float)period) +
                          (isRight ? 1.5707963f : -1.5707963f);
      w = 72;
      h = blink ? h : 40;
      cx += (int)(sinf(phase) * (float)amp);
      cy += (int)(fabsf(sinf(phase * 1.2f)) * ((float)amp * 0.35f));
      break;
    }
    case EMOTION_IDLE:
    default:
      if (!blink) {
        cy += (int)(sinf(millis() / 1200.0f + (isRight ? 0.7f : 0.0f)) * 3.0f);
      }
      break;
  }

  if (w < 10) w = 10;
  if (h < 6) h = 6;

  lv_obj_t *glow = lv_obj_create(canvas_);
  lv_obj_remove_style_all(glow);
  lv_obj_set_size(glow, w + 12, h + 12);
  lv_obj_set_pos(glow, cx - (w + 12) / 2, cy - (h + 12) / 2);
  lv_obj_set_style_radius(glow, 22, 0);
  lv_obj_set_style_bg_color(glow, glow_color, 0);
  lv_obj_set_style_bg_opa(glow, (lv_opa_t)constrain(24 + pulse_shift_ / 5, 10, 46), 0);
  if (angle != 0) lv_obj_set_style_transform_angle(glow, angle, 0);

  lv_obj_t *core = lv_obj_create(canvas_);
  lv_obj_remove_style_all(core);
  lv_obj_set_size(core, w, h);
  lv_obj_set_pos(core, cx - w / 2, cy - h / 2);
  lv_obj_set_style_radius(core, 18, 0);
  if (angle != 0) lv_obj_set_style_transform_angle(core, angle, 0);

  if (hollow) {
    lv_obj_set_style_bg_color(core, bg, 0);
    lv_obj_set_style_bg_opa(core, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(core, color, 0);
    lv_obj_set_style_border_width(core, 4, 0);
  } else {
    lv_obj_set_style_bg_color(core, color, 0);
    lv_obj_set_style_bg_opa(core, LV_OPA_COVER, 0);
  }

  if ((e == EMOTION_MAIL) && !blink) {
    lv_obj_t *flap = lv_obj_create(canvas_);
    lv_obj_remove_style_all(flap);
    lv_obj_set_size(flap, w - 10, h / 2 + 4);
    lv_obj_set_pos(flap, cx - (w - 10) / 2, cy - h / 2);
    lv_obj_set_style_bg_color(flap, bg, 0);
    lv_obj_set_style_bg_opa(flap, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(flap, color, 0);
    lv_obj_set_style_border_width(flap, 4, 0);
    lv_obj_set_style_radius(flap, 6, 0);
    lv_obj_set_style_transform_angle(flap, isRight ? 150 : -150, 0);
  }

  if ((e == EMOTION_CALL) && !blink) {
    lv_obj_t *bar = lv_obj_create(canvas_);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, 16, h - 14);
    lv_obj_set_pos(bar, cx - 8, cy - (h - 14) / 2);
    lv_obj_set_style_radius(bar, 8, 0);
    lv_obj_set_style_bg_color(bar, color, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
  }

  if (add_brow) {
    lv_obj_t *brow = lv_obj_create(canvas_);
    lv_obj_remove_style_all(brow);
    lv_obj_set_size(brow, 34, 5);
    lv_obj_set_pos(brow, cx - 17, cy - h / 2 - 16);
    lv_obj_set_style_radius(brow, 2, 0);
    lv_obj_set_style_bg_color(brow, color, 0);
    lv_obj_set_style_bg_opa(brow, LV_OPA_COVER, 0);
    lv_obj_set_style_transform_angle(brow, isRight ? 320 : -320, 0);
  }
}
void AnimeFace::drawEyeMoodLab(int16_t cx, int16_t cy, Emotion e, bool isRight, bool blink) {
  const bool angry_lab = is_angry_lab_style(style_);
  const Emotion target = angry_lab ? EMOTION_ANGRY : EMOTION_SAD;
  const int raw_variant = mood_lab_variant_index(e);
  const int variant = constrain(raw_variant, 0, 9);

  if ((e != target) && (raw_variant < 0)) {
    const FaceStyle saved_style = style_;
    style_ = FACE_STYLE_EVE_CINEMATIC;
    drawEyeEVE(cx, cy, e, isRight, blink);
    style_ = saved_style;
    return;
  }

  const MoodVariant &v = angry_lab ? kAngryVariants[variant] : kSadVariants[variant];
  const lv_color_t color = tuned_eye_color(tuning_);
  const lv_color_t glow = color;
  const lv_color_t bg = lv_color_hex(0x06080D);
  int16_t w = v.eye_w;
  int16_t h = blink ? 8 : v.eye_h;
  int16_t eye_cx = cx;
  int16_t eye_cy = cy + v.eye_y;

  if (!blink) {
    if (angry_lab) {
      if ((variant % 3) == 1) eye_cx += isRight ? -2 : 2;
      if ((variant % 3) == 2) eye_cx += isRight ? 3 : -3;
    } else {
      if ((variant % 3) == 0) eye_cy += 2;
      if ((variant % 3) == 2) eye_cx += isRight ? 2 : -2;
    }
  }

  lv_obj_t *glow_obj = lv_obj_create(canvas_);
  lv_obj_remove_style_all(glow_obj);
  lv_obj_set_size(glow_obj, w + 12, h + 12);
  lv_obj_set_pos(glow_obj, eye_cx - (w + 12) / 2, eye_cy - (h + 12) / 2);
  lv_obj_set_style_radius(glow_obj, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(glow_obj, glow, 0);
  lv_obj_set_style_bg_opa(glow_obj, (lv_opa_t)constrain(22 + pulse_shift_ / 4, 10, 58), 0);

  lv_obj_t *mid = lv_obj_create(canvas_);
  lv_obj_remove_style_all(mid);
  lv_obj_set_size(mid, w + 4, h + 4);
  lv_obj_set_pos(mid, eye_cx - (w + 4) / 2, eye_cy - (h + 4) / 2);
  lv_obj_set_style_radius(mid, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(mid, color, 0);
  lv_obj_set_style_bg_opa(mid, (lv_opa_t)constrain(92 + pulse_shift_ / 4, 40, 156), 0);

  lv_obj_t *core = lv_obj_create(canvas_);
  lv_obj_remove_style_all(core);
  lv_obj_set_size(core, w, h);
  lv_obj_set_pos(core, eye_cx - w / 2, eye_cy - h / 2);
  lv_obj_set_style_radius(core, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(core, color, 0);
  lv_obj_set_style_bg_opa(core, LV_OPA_COVER, 0);

  if (!blink && angry_lab && (v.top_cut_h > 0)) {
    lv_obj_t *cut = lv_obj_create(canvas_);
    lv_obj_remove_style_all(cut);
    lv_obj_set_size(cut, w + 24, v.top_cut_h + ((variant % 2) ? 2 : 0));
    lv_obj_set_pos(cut, eye_cx - (w + 24) / 2, eye_cy - h / 2 - 3);
    lv_obj_set_style_bg_color(cut, bg, 0);
    lv_obj_set_style_bg_opa(cut, LV_OPA_COVER, 0);
    lv_obj_set_style_transform_angle(cut, isRight ? (variant % 2 ? 60 : -60) : (variant % 2 ? -60 : 60), 0);
  }

  if (!blink && !angry_lab && (variant == 1 || variant == 3 || variant == 5 || variant == 7 || variant == 9)) {
    lv_obj_t *lid = lv_obj_create(canvas_);
    lv_obj_remove_style_all(lid);
    lv_obj_set_size(lid, w + 10, 10 + (variant % 4));
    lv_obj_set_pos(lid, eye_cx - (w + 10) / 2, eye_cy - h / 2 - 1);
    lv_obj_set_style_bg_color(lid, bg, 0);
    lv_obj_set_style_bg_opa(lid, LV_OPA_COVER, 0);
    lv_obj_set_style_transform_angle(lid, isRight ? -(120 + variant * 20) : (120 + variant * 20), 0);
  }

  const int16_t brow_angle = angry_lab ? (isRight ? v.brow_angle : -v.brow_angle)
                                       : (isRight ? -v.brow_angle : v.brow_angle);
  const int16_t brow_x = eye_cx - v.brow_w / 2 + (isRight ? v.brow_dx : -v.brow_dx);
  const int16_t brow_y = eye_cy - h / 2 - v.brow_gap - (angry_lab ? 2 : 1);

  lv_obj_t *brow_glow = lv_obj_create(canvas_);
  lv_obj_remove_style_all(brow_glow);
  lv_obj_set_size(brow_glow, v.brow_w + 10, v.brow_h + 6);
  lv_obj_set_pos(brow_glow, brow_x - 5, brow_y - 3);
  lv_obj_set_style_radius(brow_glow, 4, 0);
  lv_obj_set_style_bg_color(brow_glow, glow, 0);
  lv_obj_set_style_bg_opa(brow_glow, (lv_opa_t)(angry_lab ? 120 : 96), 0);
  lv_obj_set_style_transform_angle(brow_glow, brow_angle, 0);

  lv_obj_t *brow = lv_obj_create(canvas_);
  lv_obj_remove_style_all(brow);
  lv_obj_set_size(brow, v.brow_w, v.brow_h);
  lv_obj_set_pos(brow, brow_x, brow_y);
  lv_obj_set_style_radius(brow, 3, 0);
  lv_obj_set_style_bg_color(brow, color, 0);
  lv_obj_set_style_bg_opa(brow, LV_OPA_COVER, 0);
  lv_obj_set_style_transform_angle(brow, brow_angle, 0);

  if (!blink && variant >= 5) {
    lv_obj_t *brow_inner = lv_obj_create(canvas_);
    lv_obj_remove_style_all(brow_inner);
    lv_obj_set_size(brow_inner, (int16_t)(v.brow_w * 0.62f), (int16_t)(v.brow_h + 1));
    lv_obj_set_pos(brow_inner, brow_x + (isRight ? 0 : (v.brow_w - (int16_t)(v.brow_w * 0.62f))), brow_y);
    lv_obj_set_style_radius(brow_inner, 3, 0);
    lv_obj_set_style_bg_color(brow_inner, tuned_eye_glow(tuning_, angry_lab ? 54 : 34), 0);
    lv_obj_set_style_bg_opa(brow_inner, LV_OPA_COVER, 0);
    lv_obj_set_style_transform_angle(brow_inner, brow_angle, 0);
  }
}

