#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

#include "LVGL_Arduino/BAT_Driver.h"
#include "LVGL_Arduino/Display_ST77916.h"
#include "LVGL_Arduino/I2C_Driver.h"
#include "LVGL_Arduino/LVGL_Driver.h"
#include "LVGL_Arduino/SD_Card.h"
#include "LVGL_Arduino/TCA9554PWR.h"
#include "LayerRenderer.h"
#include "ScenePresets.h"

using namespace nse;

namespace {
LayerRenderer *g_renderer = nullptr;
lv_obj_t *g_canvas = nullptr;
lv_obj_t *g_scene_label = nullptr;
uint32_t g_last_blink_toggle_ms = 0;
bool g_blink_active = false;
bool g_auto_cycle = true;
uint32_t g_last_scene_cycle_ms = 0;
uint8_t g_backlight = 18;
lv_color_t g_eye_color = lv_color_make(0x0F, 0xDA, 0xFF);
const EyeSceneSpec *g_current_scene = &kEveBaseScene;
String g_serial_line;
int8_t g_blink_override = -1;
int8_t g_drift_override = -1;
int8_t g_pulse_override = -1;

void update_scene_label();

void print_help() {
  Serial.println("[NEXTGEN] commands:");
  Serial.println("  help");
  Serial.println("  scenes");
  Serial.println("  scene <name>");
  Serial.println("  next");
  Serial.println("  color <r> <g> <b>");
  Serial.println("  blink auto|on|off");
  Serial.println("  drift on|off|auto");
  Serial.println("  pulse on|off|auto");
  Serial.println("  backlight <0-100>");
  Serial.println("  cycle on|off");
  Serial.println("  status");
}

bool scene_blink_enabled() {
  if (!g_current_scene) return false;
  if (g_blink_override >= 0) return g_blink_override == 1;
  return g_current_scene->animation.blink.enabled;
}

bool scene_drift_enabled() {
  if (!g_current_scene) return false;
  if (g_drift_override >= 0) return g_drift_override == 1;
  return g_current_scene->animation.drift.enabled;
}

bool scene_pulse_enabled() {
  if (!g_current_scene) return false;
  if (g_pulse_override >= 0) return g_pulse_override == 1;
  return g_current_scene->animation.pulse.enabled;
}

void print_status() {
  Serial.printf("[NEXTGEN] scene=%s color=(%u,%u,%u) blink_enabled=%d blink=%d drift=%d pulse=%d cycle=%d backlight=%u\n",
                g_current_scene ? g_current_scene->name : "none",
                g_eye_color.ch.red, g_eye_color.ch.green, g_eye_color.ch.blue,
                scene_blink_enabled() ? 1 : 0, g_blink_active ? 1 : 0,
                scene_drift_enabled() ? 1 : 0, scene_pulse_enabled() ? 1 : 0,
                g_auto_cycle ? 1 : 0, g_backlight);
}

void list_scenes() {
  Serial.println("[NEXTGEN] builtin scenes:");
  for (size_t i = 0; i < kBuiltinSceneCount; ++i) {
    Serial.printf("  %s%s\n", kBuiltinScenes[i].name, (kBuiltinScenes[i].scene == g_current_scene) ? "  <current>" : "");
  }
}

const EyeSceneSpec *find_scene(const char *name) {
  for (size_t i = 0; i < kBuiltinSceneCount; ++i) {
    if (strcmp(kBuiltinScenes[i].name, name) == 0) return kBuiltinScenes[i].scene;
  }
  return nullptr;
}

void reset_scene_timers() {
  g_last_blink_toggle_ms = millis();
  g_blink_active = false;
}

void select_scene(const EyeSceneSpec *scene) {
  if (!scene) return;
  g_current_scene = scene;
  reset_scene_timers();
  update_scene_label();
  Serial.printf("[NEXTGEN] scene -> %s\n", g_current_scene->name);
}

void select_next_scene() {
  size_t current = 0;
  for (size_t i = 0; i < kBaseSceneCount; ++i) {
    if (kBaseScenes[i].scene == g_current_scene) {
      current = i;
      break;
    }
  }
  current = (current + 1) % kBaseSceneCount;
  select_scene(kBaseScenes[current].scene);
}

int8_t pulse_shift(uint32_t now_ms) {
  if (!scene_pulse_enabled()) return 0;
  const PulseProfile &pulse = g_current_scene->animation.pulse;
  if (pulse.period_ms == 0) return 0;
  const float phase = (float)(now_ms % pulse.period_ms) / (float)pulse.period_ms;
  return (int8_t)(sinf(phase * 6.2831853f) * (float)pulse.amplitude);
}

int16_t wave_value(uint32_t now_ms, uint16_t period_ms, int16_t amplitude, bool use_cos) {
  if (period_ms == 0 || amplitude == 0) return 0;
  const float phase = (float)(now_ms % period_ms) / (float)period_ms;
  const float angle = phase * 6.2831853f;
  const float value = use_cos ? cosf(angle) : sinf(angle);
  return (int16_t)(value * (float)amplitude);
}

RuntimeState make_runtime_state() {
  const uint32_t now = millis();
  if (scene_blink_enabled()) {
    const BlinkProfile &blink = g_current_scene->animation.blink;
    if (!g_blink_active && (now - g_last_blink_toggle_ms) > blink.interval_ms) {
      g_blink_active = true;
      g_last_blink_toggle_ms = now;
    } else if (g_blink_active && (now - g_last_blink_toggle_ms) > blink.duration_ms) {
      g_blink_active = false;
      g_last_blink_toggle_ms = now;
    }
  } else {
    g_blink_active = false;
  }

  RuntimeState state{};
  state.eye_color = g_eye_color;
  state.blink = g_blink_active;
  state.blink_height = g_current_scene ? g_current_scene->animation.blink.closed_height : 8;
  if (scene_drift_enabled()) {
    const DriftProfile &drift = g_current_scene->animation.drift;
    state.drift_x = wave_value(now, drift.drift_x_period_ms, drift.drift_x_amplitude, false);
    state.drift_y = wave_value(now, drift.drift_y_period_ms, drift.drift_y_amplitude, true);
    state.saccade_x = wave_value(now, drift.saccade_x_period_ms, drift.saccade_x_amplitude, false);
    state.saccade_y = wave_value(now, drift.saccade_y_period_ms, drift.saccade_y_amplitude, true);
  }
  state.pulse_shift = pulse_shift(now);
  return state;
}

void update_scene_label() {
  if (!g_scene_label) return;
  lv_label_set_text_fmt(g_scene_label, "%s", g_current_scene ? g_current_scene->name : "none");
}

void create_scene_canvas() {
  g_canvas = lv_obj_create(lv_scr_act());
  lv_obj_remove_style_all(g_canvas);
  lv_obj_clear_flag(g_canvas, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(g_canvas, LCD_WIDTH, LCD_HEIGHT);
  lv_obj_set_pos(g_canvas, 0, 0);
  lv_obj_set_style_bg_color(g_canvas, lv_color_hex(0x06080D), 0);
  lv_obj_set_style_bg_opa(g_canvas, LV_OPA_COVER, 0);

  g_scene_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(g_scene_label, lv_color_make(0x6A, 0xD7, 0xF5), 0);
  lv_obj_set_style_text_font(g_scene_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_bg_color(g_scene_label, lv_color_hex(0x06080D), 0);
  lv_obj_set_style_bg_opa(g_scene_label, LV_OPA_50, 0);
  lv_obj_set_style_pad_left(g_scene_label, 8, 0);
  lv_obj_set_style_pad_right(g_scene_label, 8, 0);
  lv_obj_set_style_pad_top(g_scene_label, 4, 0);
  lv_obj_set_style_pad_bottom(g_scene_label, 4, 0);
  lv_obj_set_style_radius(g_scene_label, 10, 0);
  lv_obj_align(g_scene_label, LV_ALIGN_BOTTOM_MID, 0, -14);
  update_scene_label();

  static LayerRenderer renderer(g_canvas);
  g_renderer = &renderer;
}

void handle_command(const String &line) {
  String cmd = line;
  cmd.trim();
  if (!cmd.length()) return;

  if (cmd.equalsIgnoreCase("help")) {
    print_help();
    return;
  }
  if (cmd.equalsIgnoreCase("scenes")) {
    list_scenes();
    return;
  }
  if (cmd.equalsIgnoreCase("next")) {
    select_next_scene();
    return;
  }
  if (cmd.equalsIgnoreCase("status")) {
    print_status();
    return;
  }
  if (cmd.startsWith("scene ")) {
    const String name = cmd.substring(6);
    if (const EyeSceneSpec *scene = find_scene(name.c_str())) {
      select_scene(scene);
    } else {
      Serial.printf("[NEXTGEN] unknown scene: %s\n", name.c_str());
    }
    return;
  }
  if (cmd.startsWith("color ")) {
    int r = -1, g = -1, b = -1;
    if (sscanf(cmd.c_str(), "color %d %d %d", &r, &g, &b) == 3) {
      r = constrain(r, 0, 255); g = constrain(g, 0, 255); b = constrain(b, 0, 255);
      g_eye_color = lv_color_make((uint8_t)r, (uint8_t)g, (uint8_t)b);
      Serial.printf("[NEXTGEN] color -> %d %d %d\n", r, g, b);
    } else {
      Serial.println("[NEXTGEN] usage: color <r> <g> <b>");
    }
    return;
  }
  if (cmd.startsWith("blink ")) {
    const String mode = cmd.substring(6);
    if (mode.equalsIgnoreCase("auto")) {
      g_blink_override = -1;
      reset_scene_timers();
    } else if (mode.equalsIgnoreCase("on")) {
      g_blink_override = 1;
      g_blink_active = true;
    } else if (mode.equalsIgnoreCase("off")) {
      g_blink_override = 0;
      g_blink_active = false;
    }
    print_status();
    return;
  }
  if (cmd.startsWith("drift ")) {
    const String mode = cmd.substring(6);
    if (mode.equalsIgnoreCase("auto")) g_drift_override = -1;
    else g_drift_override = mode.equalsIgnoreCase("on") ? 1 : 0;
    print_status();
    return;
  }
  if (cmd.startsWith("pulse ")) {
    const String mode = cmd.substring(6);
    if (mode.equalsIgnoreCase("auto")) g_pulse_override = -1;
    else g_pulse_override = mode.equalsIgnoreCase("on") ? 1 : 0;
    print_status();
    return;
  }
  if (cmd.startsWith("backlight ")) {
    int value = -1;
    if (sscanf(cmd.c_str(), "backlight %d", &value) == 1) {
      g_backlight = (uint8_t)constrain(value, 0, 100);
      Set_Backlight(g_backlight);
      Serial.printf("[NEXTGEN] backlight -> %u\n", g_backlight);
    }
    return;
  }
  if (cmd.startsWith("cycle ")) {
    const String mode = cmd.substring(6);
    g_auto_cycle = mode.equalsIgnoreCase("on");
    if (g_auto_cycle) g_last_scene_cycle_ms = millis();
    print_status();
    return;
  }

  Serial.printf("[NEXTGEN] unknown command: %s\n", cmd.c_str());
}

void apply_auto_cycle() {
  if (!g_auto_cycle) return;
  const uint32_t now = millis();
  if ((now - g_last_scene_cycle_ms) < 5000U) return;
  g_last_scene_cycle_ms = now;
  select_next_scene();
}

void poll_serial() {
  while (Serial.available() > 0) {
    const char ch = (char)Serial.read();
    if (ch == '\n' || ch == '\r') {
      if (g_serial_line.length()) {
        handle_command(g_serial_line);
        g_serial_line = "";
      }
    } else {
      g_serial_line += ch;
    }
  }
}
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("[NEXTGEN] setup start");

  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);
  delay(20);

  I2C_Init();
  TCA9554PWR_Init(0x00);
  BAT_Init();
  SD_Init();
  Flash_test();
  Backlight_Init();
  Set_Backlight(g_backlight);
  LCD_Init();
  Lvgl_Init();
  create_scene_canvas();

  Serial.println("[NEXTGEN] renderer demo ready");
  print_help();
  print_status();
  g_last_scene_cycle_ms = millis();
  reset_scene_timers();
}

void loop() {
  poll_serial();
  apply_auto_cycle();
  if (g_renderer && g_current_scene) {
    const RuntimeState state = make_runtime_state();
    g_renderer->render(*g_current_scene, state);
  }
  Lvgl_Loop();
  vTaskDelay(pdMS_TO_TICKS(16));
}




