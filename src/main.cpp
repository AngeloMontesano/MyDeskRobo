#include <Arduino.h>

#include "LVGL_Arduino/I2C_Driver.h"
#include "LVGL_Arduino/TCA9554PWR.h"
#include "LVGL_Arduino/Display_ST77916.h"
#include "LVGL_Arduino/LVGL_Driver.h"
#include "LVGL_Arduino/Gyro_QMI8658.h"
#include "LVGL_Arduino/SD_Card.h"
#include "LVGL_Arduino/BAT_Driver.h"

#include "DeskRoboMVP.h"
#include "DeskRoboWeb.h"
#include "DeskRoboBLE.h"
#include "DeskRoboAudio.h"

#ifndef DESKROBO_ENABLE_GYRO
#define DESKROBO_ENABLE_GYRO 0
#endif

#ifndef DESKROBO_GYRO_SAFE_PROBE
#define DESKROBO_GYRO_SAFE_PROBE 0
#endif

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
    Serial.printf("[GYRO] addr=0x%02X who_ok=%d who=0x%02X rev_ok=%d rev=0x%02X\n",
                  addr, who_ok ? 1 : 0, who, rev_ok ? 1 : 0, rev);
    if (who_ok && (who != 0x00) && (who != 0xFF)) {
      found = true;
    }
  }
  Serial.printf("[GYRO] safe probe result: %s\n", found ? "sensor found" : "not found");
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("[BOOT] setup start");

  // Keep board power rail enabled in minimal display-only bring-up.
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
  Serial.println("[BOOT] Gyro init done");
#else
  Serial.println("[BOOT] Gyro init skipped");
#endif
  Backlight_Init();
  Set_Backlight(65);
  Serial.println("[BOOT] backlight set");
  LCD_Init();
  Serial.println("[BOOT] LCD init done");
  Lvgl_Init();
  Serial.println("[BOOT] LVGL init done");
  DeskRobo_Init();
  Serial.println("[BOOT] DeskRobo MVP ready");
  DeskRoboWeb_Init();
  Serial.println("[BOOT] DeskRobo Web ready");
  DeskRoboBLE_Init();
  Serial.println("[BOOT] DeskRobo BLE ready");
  DeskRoboAudio_Init();
  Serial.println("[BOOT] DeskRobo Audio test ready");
}

void loop() {
  DeskRobo_Loop();
  DeskRoboAudio_Loop();
  DeskRoboWeb_Loop();
  DeskRoboBLE_Loop();
  Lvgl_Loop();
  vTaskDelay(pdMS_TO_TICKS(5));
}
