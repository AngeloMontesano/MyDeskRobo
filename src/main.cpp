#include <Arduino.h>

#include "LVGL_Arduino/BAT_Driver.h"
#include "LVGL_Arduino/Display_ST77916.h"
#include "LVGL_Arduino/Gyro_QMI8658.h"
#include "LVGL_Arduino/I2C_Driver.h"
#include "LVGL_Arduino/LVGL_Driver.h"
#include "LVGL_Arduino/SD_Card.h"
#include "LVGL_Arduino/TCA9554PWR.h"
#include "DeskRoboAudio.h"
#include "DeskRoboBLE.h"
#include "DeskRoboMVP.h"

#ifndef DESKROBO_ENABLE_GYRO
#define DESKROBO_ENABLE_GYRO 0
#endif

#ifndef DESKROBO_GYRO_SAFE_PROBE
#define DESKROBO_GYRO_SAFE_PROBE 0
#endif

static volatile bool g_ble_ready = false;
static uint32_t g_last_motion_poll_ms = 0;
static uint32_t g_last_shake_event_ms = 0;
static uint32_t g_motion_boot_grace_until_ms = 0;
static uint8_t g_shake_hits = 0;
static bool g_motion_seeded = false;
static float g_prev_ax = 0.0f;
static float g_prev_ay = 0.0f;
static float g_prev_az = 1.0f;

static void GyroMotionLoop() {
#if DESKROBO_ENABLE_GYRO
  if (!QMI8658_IsReady()) return;
  const uint32_t now = millis();
  if (now < g_motion_boot_grace_until_ms) return;
  if ((now - g_last_motion_poll_ms) < 40U) return;
  g_last_motion_poll_ms = now;

  QMI8658_Loop();
  getGyroscope();

  const float ax = Accel.x;
  const float ay = Accel.y;
  const float az = Accel.z;
  const float gx = Gyro.x;
  const float gy = Gyro.y;
  const float gz = Gyro.z;
  if (isnan(ax) || isnan(ay) || isnan(az) || isnan(gx) || isnan(gy) || isnan(gz)) return;

  if (!g_motion_seeded) {
    g_prev_ax = ax;
    g_prev_ay = ay;
    g_prev_az = az;
    g_motion_seeded = true;
    return;
  }

  const float delta = fabsf(ax - g_prev_ax) + fabsf(ay - g_prev_ay) + fabsf(az - g_prev_az);
  const float gyro_sum = fabsf(gx) + fabsf(gy) + fabsf(gz);
  const bool shake_sample = (delta > 1.10f) || (gyro_sum > 42.0f);

  g_prev_ax = ax;
  g_prev_ay = ay;
  g_prev_az = az;

  if (shake_sample) {
    if (g_shake_hits < 3) g_shake_hits++;
  } else {
    g_shake_hits = 0;
  }

  if (g_shake_hits >= 2 && (now - g_last_shake_event_ms) > 2600U) {
    g_last_shake_event_ms = now;
    g_shake_hits = 0;
    DeskRobo_PushEvent(DESKROBO_EVENT_MOTION_SHAKE);
    Serial.printf("[GYRO] SHAKE ax=%.2f ay=%.2f az=%.2f gx=%.2f gy=%.2f gz=%.2f\n", ax, ay, az, gx, gy, gz);
  }
#endif
}
static volatile bool g_ble_init_requested = false;

static void BleInitTask(void *param) {
  (void)param;
  DeskRoboBLE_Init();
  g_ble_ready = true;
  Serial.println("[BOOT] MyDeskRobo BLE ready");
  vTaskDelete(nullptr);
}

static void RequestBleInit() {
  if (g_ble_ready || g_ble_init_requested) {
    return;
  }

  g_ble_init_requested = true;
  const BaseType_t ble_task_ok =
      xTaskCreate(BleInitTask, "ble_init", 12288, nullptr, 1, nullptr);
  if (ble_task_ok == pdPASS) {
    Serial.println("[BOOT] MyDeskRobo BLE init task started");
  } else {
    g_ble_init_requested = false;
    Serial.println("[BOOT] MyDeskRobo BLE init task failed (BLE disabled)");
  }
}

static void Gyro_SafeProbe() {
  uint8_t who = 0;
  uint8_t rev = 0;
  const uint8_t addrs[2] = {QMI8658_L_SLAVE_ADDRESS, QMI8658_H_SLAVE_ADDRESS};
  bool found = false;
  Serial.println("[GYRO] safe probe start");
  for (uint8_t i = 0; i < 2; ++i) {
    const uint8_t addr = addrs[i];
    const bool who_ok = !I2C_Read(addr, QMI8658_WHO_AM_I, &who, 1);
    const bool rev_ok = !I2C_Read(addr, QMI8658_REVISION_ID, &rev, 1);
    Serial.printf(
        "[GYRO] addr=0x%02X who_ok=%d who=0x%02X rev_ok=%d rev=0x%02X\n", addr,
        who_ok ? 1 : 0, who, rev_ok ? 1 : 0, rev);
    if (who_ok && (who != 0x00) && (who != 0xFF)) {
      found = true;
    }
  }
  Serial.printf("[GYRO] safe probe result: %s\n",
                found ? "sensor found" : "not found");
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("[BOOT] setup start");

  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);
  delay(20);
  Serial.println("[BOOT] power hold set");

  I2C_Init();
  Serial.println("[BOOT] I2C init done");
  TCA9554PWR_Init(0x00);
  Serial.println("[BOOT] EXIO init done");
  BAT_Init();
  Serial.println("[BOOT] BAT init done");
  SD_Init();
  Flash_test();
  Serial.printf("[BOOT] SD size=%uMB Flash=%uMB\n", SDCard_Size, Flash_Size);
#if DESKROBO_GYRO_SAFE_PROBE
  Gyro_SafeProbe();
#endif
#if DESKROBO_ENABLE_GYRO
  QMI8658_Init();
  g_motion_boot_grace_until_ms = millis() + 6000U;
  Serial.println("[BOOT] Gyro init done");
#else
  Serial.println("[BOOT] Gyro init skipped");
#endif
  Backlight_Init();
  Set_Backlight(15);
  Serial.println("[BOOT] backlight set");
  LCD_Init();
  Serial.println("[BOOT] LCD init done");
  Lvgl_Init();
  Serial.println("[BOOT] LVGL init done");
  DeskRobo_Init();
  Serial.println("[BOOT] MyDeskRobo MVP ready");
  DeskRoboAudio_Init();
  Serial.println("[BOOT] MyDeskRobo Audio test ready");
  RequestBleInit();
}

void loop() {
  DeskRobo_Loop();
  DeskRoboAudio_Loop();
  if (g_ble_ready) {
    DeskRoboBLE_Loop();
  }
  GyroMotionLoop();
  Lvgl_Loop();
  vTaskDelay(pdMS_TO_TICKS(5));
}


