#include <Arduino.h>

#include "LVGL_Arduino/I2C_Driver.h"
#include "LVGL_Arduino/TCA9554PWR.h"
#include "LVGL_Arduino/Display_ST77916.h"
#include "LVGL_Arduino/LVGL_Driver.h"
#include "LVGL_Arduino/Gyro_QMI8658.h"

#include "DeskRoboMVP.h"
#include "DeskRoboWeb.h"
#include "DeskRoboBLE.h"

#ifndef DESKROBO_ENABLE_GYRO
#define DESKROBO_ENABLE_GYRO 0
#endif

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
}

void loop() {
  DeskRobo_Loop();
  DeskRoboWeb_Loop();
  DeskRoboBLE_Loop();
  Lvgl_Loop();
  vTaskDelay(pdMS_TO_TICKS(5));
}
